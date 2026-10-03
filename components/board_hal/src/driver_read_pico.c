#include <string.h>

#include "board_hal.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "read_pico_board.h"
#include "read_pico_pmu_protocol.h"

static const char *TAG = "read_pico_board";
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t pmu;
static i2c_master_dev_handle_t ioe;
static read_pico_board_snapshot_t snapshot;
static int64_t snapshot_time;
static bool snapshot_valid;
static StaticSemaphore_t mutex_storage;
static SemaphoreHandle_t mutex;
static bool display_claimed;

esp_err_t read_pico_board_lock(uint32_t timeout_ms)
{
    if (mutex == NULL)
        return ESP_ERR_INVALID_STATE;
    return xSemaphoreTakeRecursive(mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE ? ESP_OK
                                                                               : ESP_ERR_TIMEOUT;
}

void read_pico_board_unlock(void)
{
    if (mutex != NULL)
        xSemaphoreGiveRecursive(mutex);
}

static bool display_access(void)
{
    return display_claimed && mutex != NULL &&
           xSemaphoreGetMutexHolder(mutex) == xTaskGetCurrentTaskHandle();
}

esp_err_t read_pico_board_display_claim(i2c_master_bus_handle_t *out)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    *out = NULL;
    if (mutex == NULL || xSemaphoreGetMutexHolder(mutex) != xTaskGetCurrentTaskHandle() ||
        bus == NULL || display_claimed)
        return ESP_ERR_INVALID_STATE;
    display_claimed = true;
    *out = bus;
    return ESP_OK;
}

esp_err_t read_pico_board_display_release(void)
{
    if (!display_access())
        return ESP_ERR_INVALID_STATE;
    display_claimed = false;
    return ESP_OK;
}

static esp_err_t read_reg(i2c_master_dev_handle_t device, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(device, &reg, 1, data, len, 100);
}

static esp_err_t add_device(uint16_t address, i2c_master_dev_handle_t *device)
{
    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };
    return i2c_master_bus_add_device(bus, &config, device);
}

static esp_err_t deinit_locked(void)
{
    if (display_claimed)
        return ESP_ERR_INVALID_STATE;
    snapshot_valid = false;
    memset(&snapshot, 0, sizeof(snapshot));
    if (pmu != NULL) {
        esp_err_t err = i2c_master_bus_rm_device(pmu);
        if (err != ESP_OK)
            return err;
        pmu = NULL;
    }
    if (ioe != NULL) {
        esp_err_t err = i2c_master_bus_rm_device(ioe);
        if (err != ESP_OK)
            return err;
        ioe = NULL;
    }
    if (bus != NULL) {
        esp_err_t err = i2c_del_master_bus(bus);
        if (err != ESP_OK)
            return err;
        bus = NULL;
    }
    return ESP_OK;
}

esp_err_t read_pico_board_deinit(void)
{
    if (mutex == NULL)
        return ESP_OK;
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    err = deinit_locked();
    read_pico_board_unlock();
    return err;
}

esp_err_t read_pico_board_ioe_read(uint8_t reg, uint8_t *value)
{
    if (value == NULL || (reg != 0 && reg != 2 && reg != 6))
        return ESP_ERR_INVALID_ARG;
    if (!display_access())
        return ESP_ERR_INVALID_STATE;
    return read_reg(ioe, reg, value, 1);
}

static esp_err_t ioe_write_verified(uint8_t reg, uint8_t value)
{
    uint8_t request[2] = {reg, value}, actual = 0;
    esp_err_t err = i2c_master_transmit(ioe, request, sizeof(request), 100);
    if (err == ESP_OK)
        err = read_reg(ioe, reg, &actual, 1);
    if (err == ESP_OK && actual != value)
        err = ESP_ERR_INVALID_RESPONSE;
    return err;
}

esp_err_t read_pico_board_ioe_update(uint8_t mask, uint8_t value)
{
    // Only MODE/XOE/SY_EN/VCOM_EN. Preserve TP_RST, SD_CD and PMU interrupt bits.
    if ((mask & ~0x1b) != 0 || (value & ~mask) != 0)
        return ESP_ERR_INVALID_ARG;
    if (!display_access())
        return ESP_ERR_INVALID_STATE;
    uint8_t old;
    esp_err_t err = read_reg(ioe, 2, &old, 1);
    return err == ESP_OK ? ioe_write_verified(2, (old & ~mask) | value) : err;
}

esp_err_t read_pico_board_ioe_configure_display(void)
{
    if (!display_access())
        return ESP_ERR_INVALID_STATE;
    uint8_t config, output;
    esp_err_t err = read_reg(ioe, 6, &config, 1);
    if (err == ESP_OK)
        err = read_reg(ioe, 2, &output, 1);
    if (err != ESP_OK)
        return err;
    // Do not take over live rails or repurpose input signals. Touch direction is preserved.
    if ((config & 0x64) != 0x64 || ((~config & output) & 0x18) != 0)
        return ESP_ERR_INVALID_STATE;
    // Set safe output latches BEFORE changing direction, avoiding power-on glitches.
    err = ioe_write_verified(2, (output & ~0x1b) | 0x01);
    if (err == ESP_OK)
        err = ioe_write_verified(6, config & ~0x1b);
    return err;
}

esp_err_t read_pico_board_get_snapshot(read_pico_board_snapshot_t *out)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    out->vcom_mv = -1;
    out->status.battery_mv = -1;
    out->status.battery_percent = -1;
    out->status.charging = -1;
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    if (!snapshot_valid || esp_timer_get_time() - snapshot_time > 5000000)
        err = ESP_ERR_INVALID_STATE;
    else
        *out = snapshot;
    read_pico_board_unlock();
    return err;
}

static esp_err_t refresh_locked(void)
{
    snapshot_valid = false;
    memset(&snapshot, 0, sizeof(snapshot));
    snapshot.vcom_mv = -1;
    if (bus == NULL || pmu == NULL || ioe == NULL)
        return ESP_ERR_INVALID_STATE;

    uint8_t identity[PMU_IDENTITY_SIZE];
    uint8_t status[PMU_STATUS_SIZE];
    esp_err_t err = read_reg(pmu, PMU_REG_IDENTITY, identity, sizeof(identity));
    if (err != ESP_OK)
        return err;
    if (!read_pico_decode_identity(identity, sizeof(identity), &snapshot.identity)) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    // The fixed vendor parser documents torn STATUS shadow updates mid-read.
    // Re-read ONLY an invalid CRC before sending any GET; never publish a torn sample,
    // mask an I/O error, or retry a command under a new sequence/session.
    bool status_valid = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        if (attempt > 0)
            vTaskDelay(pdMS_TO_TICKS(attempt == 1 ? 2 : 5));
        err = read_reg(pmu, PMU_REG_STATUS, status, sizeof(status));
        if (err != ESP_OK)
            return err;
        if (read_pico_decode_status(status, sizeof(status), &snapshot.identity, &snapshot.status)) {
            status_valid = true;
            break;
        }
        ESP_LOGW(TAG, "STATUS CRC rejected (read %d/3); no sample/command published", attempt + 1);
    }
    if (!status_valid)
        return ESP_ERR_INVALID_CRC;

    // Read only CFG0 and OUT0. Never change MODE/XOE/SY_EN/VCOM_EN/directions.
    err = read_reg(ioe, 6, &snapshot.ioe_config, 1);
    if (err != ESP_OK)
        return err;
    err = read_reg(ioe, 2, &snapshot.ioe_output, 1);
    if (err != ESP_OK)
        return err;
    snapshot.sy_responding = i2c_master_probe(bus, 0x62, 100) == ESP_OK;

    uint16_t sequence = read_pico_next_sequence(snapshot.status.last_sequence);
    uint8_t request[1 + PMU_FRAME_SIZE] = {PMU_REG_COMMAND};
    if (!read_pico_encode_vcom_get(&snapshot.identity, sequence, request + 1, PMU_FRAME_SIZE)) {
        return ESP_ERR_INVALID_STATE;
    }
    // A GET uses a wire-level command write, but does not write calibration or power state.
    err = i2c_master_transmit(pmu, request, sizeof(request), 100);
    if (err != ESP_OK)
        return err;
    uint8_t response[PMU_FRAME_SIZE];
    for (int attempt = 0; attempt < 5; ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(30));
        err = read_reg(pmu, PMU_REG_RESPONSE, response, sizeof(response));
        if (err != ESP_OK)
            return err;
        if (read_pico_decode_vcom(response, sizeof(response), &snapshot.identity, sequence,
                                  &snapshot.vcom_mv)) {
            snapshot.vcom_valid = true;
            break;
        }
    }
    // Detect a PMU reboot between STATUS and GET; do not publish mixed-session data.
    read_pico_identity_t final_identity;
    err = read_reg(pmu, PMU_REG_IDENTITY, identity, sizeof(identity));
    if (err != ESP_OK)
        return err;
    if (!read_pico_decode_identity(identity, sizeof(identity), &final_identity) ||
        final_identity.session != snapshot.identity.session) {
        return ESP_ERR_INVALID_STATE;
    }
    snapshot_time = esp_timer_get_time();
    snapshot_valid = true;
    return ESP_OK;
}

esp_err_t read_pico_board_refresh(void)
{
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    err = refresh_locked();
    read_pico_board_unlock();
    return err;
}

esp_err_t board_hal_init(void)
{
    // Called once at startup before other tasks borrow the board.
    if (mutex == NULL)
        mutex = xSemaphoreCreateRecursiveMutexStatic(&mutex_storage);
    if (mutex == NULL)
        return ESP_ERR_NO_MEM;
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    if (bus != NULL) {
        read_pico_board_unlock();
        return ESP_ERR_INVALID_STATE;
    }
    i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_HAL_I2C_SDA_PIN,
        .scl_io_num = BOARD_HAL_I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    err = i2c_new_master_bus(&config, &bus);
    if (err == ESP_OK)
        err = add_device(PMU_I2C_ADDR, &pmu);
    if (err == ESP_OK)
        err = add_device(0x24, &ioe);
    if (err == ESP_OK)
        err = refresh_locked();
    if (err != ESP_OK) {
        esp_err_t cleanup = deinit_locked();
        if (cleanup != ESP_OK)
            ESP_LOGE(TAG, "Cleanup failed: %s", esp_err_to_name(cleanup));
    }
    read_pico_board_unlock();
    return err;
}

int board_hal_get_battery_voltage(void)
{
    read_pico_board_snapshot_t state;
    return read_pico_board_get_snapshot(&state) == ESP_OK ? state.status.battery_mv : -1;
}

int board_hal_get_battery_percent(void)
{
    read_pico_board_snapshot_t state;
    return read_pico_board_get_snapshot(&state) == ESP_OK ? state.status.battery_percent : -1;
}

bool board_hal_is_battery_connected(void)
{
    return board_hal_get_battery_voltage() > 0;
}
bool board_hal_is_charging(void)
{
    read_pico_board_snapshot_t state;
    return read_pico_board_get_snapshot(&state) == ESP_OK && state.status.charging == 1;
}

// USB presence cannot be inferred from USB-JTAG or NOT_CHARGING/FULL. The diagnostic
// app uses the tri-state snapshot and never runs the generic power policy.
bool board_hal_is_usb_connected(void)
{
    return false;
}
esp_err_t board_hal_prepare_for_sleep(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
void board_hal_shutdown(void)
{
    ESP_LOGE(TAG, "Shutdown disabled in P1 diagnostic");
}
esp_err_t board_hal_get_temperature(float *t)
{
    (void) t;
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t board_hal_get_humidity(float *h)
{
    (void) h;
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t board_hal_rtc_init(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t board_hal_rtc_get_time(time_t *t)
{
    (void) t;
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t board_hal_rtc_set_time(time_t t)
{
    (void) t;
    return ESP_ERR_NOT_SUPPORTED;
}
bool board_hal_rtc_is_available(void)
{
    return false;
}
void board_hal_led_set(board_hal_led_t led, bool on)
{
    (void) led;
    (void) on;
}
