#include "read_pico_pmu_session.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "read_pico_board.h"
#include "read_pico_pmu_command.h"
#include "read_pico_pmu_protocol.h"

#define SESSION_LOCK_MS 5000      // wait out a photo refresh
#define SESSION_I2C_TIMEOUT_MS 100
#define RESPONSE_POLLS 5
#define RESPONSE_POLL_MS 30
#define STATE_WAIT_MS 1000
// The PMU drops EN ~100 ms after latching; still running after this means it did not.
#define POWER_CUT_GRACE_MS 5000

static const char *TAG = "pmu_session";

static esp_err_t read_raw(i2c_master_dev_handle_t pmu, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(pmu, &reg, 1, data, len, SESSION_I2C_TIMEOUT_MS);
}

static read_pico_pmu_supply_t classify(const uint8_t *raw)
{
    uint32_t flags = (uint32_t) raw[4] | (uint32_t) raw[5] << 8 | (uint32_t) raw[6] << 16 |
                     (uint32_t) raw[7] << 24;
    if (raw[16] == PMU_CHARGE_CHARGING && (flags & PMU_STATUS_CHARGING_ACTIVE))
        return READ_PICO_PMU_SUPPLY_EXTERNAL;
    // Assumption to confirm with USB in/out readings: a full battery on USB
    // reports FULL_INFERRED (unknown), so only a plain NOT_CHARGING with the
    // charger pin low counts as battery.
    if (raw[16] == PMU_CHARGE_NOT_CHARGING && !(flags & PMU_STATUS_CHARGE_PIN_HIGH) &&
        (flags & PMU_STATUS_BATTERY_VALID))
        return READ_PICO_PMU_SUPPLY_BATTERY;
    return READ_PICO_PMU_SUPPLY_UNKNOWN;
}

// Caller holds the board lock.
static esp_err_t read_state_locked(i2c_master_dev_handle_t pmu, read_pico_identity_t *identity,
                                   read_pico_status_t *status, read_pico_pmu_state_t *out)
{
    uint8_t id_raw[PMU_IDENTITY_SIZE], st_raw[PMU_STATUS_SIZE];
    esp_err_t err = read_raw(pmu, PMU_REG_IDENTITY, id_raw, sizeof(id_raw));
    if (err != ESP_OK)
        return err;
    if (!read_pico_decode_identity(id_raw, sizeof(id_raw), identity))
        return ESP_ERR_INVALID_RESPONSE;
    for (int attempt = 0; attempt < 3; ++attempt) {  // torn shadow reads: re-read on CRC only
        err = read_raw(pmu, PMU_REG_STATUS, st_raw, sizeof(st_raw));
        if (err != ESP_OK)
            return err;
        if (read_pico_decode_status(st_raw, sizeof(st_raw), identity, status)) {
            if (out) {
                *out = (read_pico_pmu_state_t) {
                    .power_state = st_raw[2], .wake_reason = st_raw[3], .charge_state = st_raw[16],
                    .flags = (uint32_t) st_raw[4] | (uint32_t) st_raw[5] << 8 |
                             (uint32_t) st_raw[6] << 16 | (uint32_t) st_raw[7] << 24,
                    .battery_mv = status->battery_mv, .battery_percent = status->battery_percent,
                    .supply = classify(st_raw),
                };
            }
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2 + 3 * attempt));
    }
    return ESP_ERR_INVALID_CRC;
}

// One command under one fresh (session, sequence). A lost response is re-read
// with the same sequence; never re-sent under a new one (PMU idempotency).
static esp_err_t command_locked(i2c_master_dev_handle_t pmu, uint16_t opcode, const uint8_t *payload,
                                size_t length, uint16_t *status_out)
{
    read_pico_identity_t identity;
    read_pico_status_t status;
    esp_err_t err = read_state_locked(pmu, &identity, &status, NULL);
    if (err != ESP_OK)
        return err;
    uint16_t sequence = read_pico_next_sequence(status.last_sequence);
    uint8_t request[1 + 64] = {PMU_REG_COMMAND};
    if (!read_pico_pmu_encode_request(&identity, sequence, opcode, payload, length, request + 1, 64))
        return ESP_ERR_INVALID_ARG;
    err = i2c_master_transmit(pmu, request, sizeof(request), SESSION_I2C_TIMEOUT_MS);
    if (err != ESP_OK)
        return err;
    uint8_t response[64];
    for (int poll = 0; poll < RESPONSE_POLLS; ++poll) {
        vTaskDelay(pdMS_TO_TICKS(RESPONSE_POLL_MS));
        err = read_raw(pmu, PMU_REG_RESPONSE, response, sizeof(response));
        if (err != ESP_OK)
            return err;
        uint16_t code;
        if (read_pico_pmu_decode_response(response, sizeof(response), &identity, sequence, opcode, &code,
                                          NULL, 0, NULL)) {
            if (status_out) *status_out = code;
            return code == PMU_STATUS_OK || code == PMU_STATUS_ACCEPTED ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
        }
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t wait_power_locked(i2c_master_dev_handle_t pmu, uint8_t want)
{
    read_pico_identity_t identity;
    read_pico_status_t status;
    read_pico_pmu_state_t state;
    for (int waited = 0; waited <= STATE_WAIT_MS; waited += 20) {
        if (read_state_locked(pmu, &identity, &status, &state) == ESP_OK && state.power_state == want)
            return ESP_OK;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t with_pmu(i2c_master_dev_handle_t *pmu)
{
    esp_err_t err = read_pico_board_lock(SESSION_LOCK_MS);
    if (err != ESP_OK)
        return err;
    err = read_pico_board_pmu_device(pmu);
    if (err != ESP_OK)
        read_pico_board_unlock();
    return err;
}

esp_err_t read_pico_pmu_session_read(read_pico_pmu_state_t *out)
{
    if (!out)
        return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    i2c_master_dev_handle_t pmu;
    esp_err_t err = with_pmu(&pmu);
    if (err != ESP_OK)
        return err;
    read_pico_identity_t identity;
    read_pico_status_t status;
    err = read_state_locked(pmu, &identity, &status, out);
    read_pico_board_unlock();
    return err;
}

static esp_err_t ready_locked(i2c_master_dev_handle_t pmu)
{
    read_pico_identity_t identity;
    read_pico_status_t status;
    read_pico_pmu_state_t state;
    esp_err_t err = read_state_locked(pmu, &identity, &status, &state);
    if (err != ESP_OK || state.power_state == PMU_PWR_RUNNING)
        return err;
    const uint8_t boot_reason = 0;
    uint16_t code = 0;
    err = command_locked(pmu, PMU_CMD_HOST_READY, &boot_reason, 1, &code);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "HOST_READY refused: %s status=0x%04x", esp_err_to_name(err), code);
        return err;
    }
    return wait_power_locked(pmu, PMU_PWR_RUNNING);
}

esp_err_t read_pico_pmu_session_ready(void)
{
    i2c_master_dev_handle_t pmu;
    esp_err_t err = with_pmu(&pmu);
    if (err != ESP_OK)
        return err;
    err = ready_locked(pmu);
    read_pico_board_unlock();
    return err;
}

esp_err_t read_pico_pmu_session_sleep(uint32_t alarm_seconds)
{
    uint8_t payload[8];
    size_t length = 0;
    if (alarm_seconds && !(length = read_pico_pmu_alarm_once_payload(alarm_seconds, payload)))
        return ESP_ERR_INVALID_ARG;
    i2c_master_dev_handle_t pmu;
    esp_err_t err = with_pmu(&pmu);
    if (err != ESP_OK)
        return err;
    uint16_t code = 0;
    err = ready_locked(pmu);  // SOFT_SLEEP is accepted only in RUNNING
    if (err == ESP_OK && length)
        err = command_locked(pmu, PMU_CMD_ALARM_SET, payload, length, &code);
    if (err == ESP_OK)
        err = command_locked(pmu, PMU_CMD_HOST_SOFT_SLEEP, NULL, 0, &code);
    if (err == ESP_OK)
        err = wait_power_locked(pmu, PMU_PWR_SOFT_SLEEP);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Soft sleep not latched: %s status=0x%04x", esp_err_to_name(err), code);
        read_pico_board_unlock();
        return err;
    }
    // Keep the lock: nothing may start a refresh or SD write before EN drops.
    vTaskDelay(pdMS_TO_TICKS(POWER_CUT_GRACE_MS));
    // Still running: the PMU latched SOFT_SLEEP but never cut power. While EN
    // is high, HOST_READY cancels the pending power-down and returns RUNNING.
    esp_err_t cancel = ready_locked(pmu);
    ESP_LOGE(TAG, "SOFT_SLEEP latched but power not cut after %d ms; cancel: %s", POWER_CUT_GRACE_MS,
             esp_err_to_name(cancel));
    read_pico_board_unlock();
    return ESP_ERR_INVALID_STATE;
}
