#include "read_pico_frame_sleep.h"

#include <stdatomic.h>

#include "display_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "power_manager.h"
#include "read_pico_ota.h"
#include "read_pico_pmu_session.h"
#include "read_pico_sd.h"
#include "sleep_policy.h"

#define SLEEP_NVS_NAMESPACE "rp_sleep"
#define SLEEP_NVS_ENABLED "enabled"
#define SLEEP_CHECK_PERIOD_MS 5000
#define SLEEP_RETRY_AFTER_FAILURE_MS (5 * 60 * 1000)
#define PMU_WAKE_REASON_ALARM 6

static const char *TAG = "frame_sleep";
static bool s_enabled;
static bool s_woke_by_alarm;
static uint8_t s_wake_reason;
static bool s_host_ready;
static atomic_llong s_last_activity_ms;
static int64_t s_next_check_ms;
static read_pico_pmu_state_t s_state;
static bool s_state_valid;
static const char *s_last_decision = "not evaluated";

static int64_t uptime_ms(void)
{
    return esp_timer_get_time() / 1000;
}

void frame_sleep_boot(void)
{
    nvs_handle_t nvs;
    if (nvs_open(SLEEP_NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        uint8_t value = 0;
        if (nvs_get_u8(nvs, SLEEP_NVS_ENABLED, &value) == ESP_OK)
            s_enabled = value != 0;
        nvs_close(nvs);
    }
    read_pico_pmu_state_t state;
    if (read_pico_pmu_session_read(&state) == ESP_OK) {
        s_wake_reason = state.wake_reason;
        s_woke_by_alarm = state.wake_reason == PMU_WAKE_REASON_ALARM;
    }
    esp_err_t err = read_pico_pmu_session_ready();
    s_host_ready = err == ESP_OK;
    ESP_LOGI(TAG, "PMU wake reason %u, HOST_READY %s, frame sleep %s", s_wake_reason,
             esp_err_to_name(err), s_enabled ? "enabled" : "disabled");
}

bool frame_sleep_woke_by_alarm(void)
{
    return s_woke_by_alarm;
}

void frame_sleep_note_activity(void)
{
    atomic_store(&s_last_activity_ms, uptime_ms());
}

bool frame_sleep_enabled(void)
{
    return s_enabled;
}

esp_err_t frame_sleep_set_enabled(bool enabled)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(SLEEP_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
        return err;
    err = nvs_set_u8(nvs, SLEEP_NVS_ENABLED, enabled ? 1 : 0);
    if (err == ESP_OK)
        err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err == ESP_OK) {
        s_enabled = enabled;
        frame_sleep_note_activity();  // never drop off right after the switch
    }
    return err;
}

static sleep_power_t power_class(const read_pico_pmu_state_t *state)
{
    switch (state->supply) {
    case READ_PICO_PMU_SUPPLY_BATTERY: return SLEEP_POWER_BATTERY;
    case READ_PICO_PMU_SUPPLY_EXTERNAL: return SLEEP_POWER_EXTERNAL;
    default: return SLEEP_POWER_UNKNOWN;
    }
}

void frame_sleep_tick(int64_t now_ms)
{
    if (now_ms < s_next_check_ms)
        return;
    s_next_check_ms = now_ms + SLEEP_CHECK_PERIOD_MS;
    s_state_valid = read_pico_pmu_session_read(&s_state) == ESP_OK;
    if (!s_state_valid) {
        s_last_decision = "PMU state unreadable";
        return;
    }
    read_pico_sd_status_t sd;
    bool sd_busy = read_pico_sd_get_status(&sd) != ESP_OK || sd.active_io != 0;
    sleep_inputs_t in = {
        .enabled = s_enabled && s_host_ready,
        .power = power_class(&s_state),
        .woke_by_alarm = s_woke_by_alarm,
        .uptime_ms = now_ms,
        .last_activity_ms = atomic_load(&s_last_activity_ms),
        .busy = display_manager_is_busy() || sd_busy || read_pico_ota_busy(),
        .seconds_to_next = power_manager_seconds_to_next_rotation(),
    };
    sleep_decision_t decision = sleep_decide(&in);
    if (!decision.sleep) {
        s_last_decision = !in.enabled ? "disabled" : in.power != SLEEP_POWER_BATTERY ? "not on battery"
                          : in.busy ? "busy" : "awake window";
        return;
    }
    ESP_LOGW(TAG, "Sleeping on battery (%d mV); wake in %lu s%s", s_state.battery_mv,
             (unsigned long) decision.alarm_seconds, decision.alarm_seconds ? "" : " (key only)");
    esp_err_t err = read_pico_pmu_session_sleep(decision.alarm_seconds);
    if (err == ESP_OK) {
        // The PMU drops EN about 100 ms after latching; the board lock stays held.
        s_last_decision = "sleeping";
        vTaskDelay(portMAX_DELAY);
    }
    s_last_decision = "sleep refused by PMU";
    s_next_check_ms = now_ms + SLEEP_RETRY_AFTER_FAILURE_MS;
}

void frame_sleep_describe(cJSON *object)
{
    cJSON_AddBoolToObject(object, "frame_sleep_enabled", s_enabled);
    cJSON_AddBoolToObject(object, "host_ready", s_host_ready);
    cJSON_AddNumberToObject(object, "wake_reason", s_wake_reason);
    cJSON_AddStringToObject(object, "last_decision", s_last_decision);
    if (!s_state_valid)
        return;
    static const char *const supply[] = {"unknown", "external", "battery"};
    cJSON_AddStringToObject(object, "supply", supply[s_state.supply]);
    cJSON_AddNumberToObject(object, "battery_mv", s_state.battery_mv);
    cJSON_AddNumberToObject(object, "battery_percent", s_state.battery_percent);
    cJSON_AddNumberToObject(object, "charge_state", s_state.charge_state);
    cJSON_AddNumberToObject(object, "pmu_flags", s_state.flags);
    cJSON_AddNumberToObject(object, "power_state", s_state.power_state);
}
