#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
// Opt-in PMU host session for the sleep frame (CONFIG_READ_PICO_PMU_SLEEP).
// Every call takes the board lock; none of them writes VCOM, configuration
// or calibration. Only HOST_READY, ALARM_SET (once) and HOST_SOFT_SLEEP are sent.
typedef enum {
    READ_PICO_PMU_SUPPLY_UNKNOWN,
    READ_PICO_PMU_SUPPLY_EXTERNAL,  // charging
    READ_PICO_PMU_SUPPLY_BATTERY,   // not charging and charge pin low (to confirm on device)
} read_pico_pmu_supply_t;

typedef struct {
    uint8_t power_state;   // PMU_PWR_*
    uint8_t wake_reason;   // 0 unknown .. 6 alarm
    uint8_t charge_state;  // PMU_CHARGE_*
    uint32_t flags;
    int battery_mv;
    int battery_percent;
    read_pico_pmu_supply_t supply;
} read_pico_pmu_state_t;

esp_err_t read_pico_pmu_session_read(read_pico_pmu_state_t *out);
// HOST_READY (boot_reason 0) unless already RUNNING; waits up to 1 s for RUNNING.
esp_err_t read_pico_pmu_session_ready(void);
// alarm_seconds 0: no alarm (key wake only); otherwise 10 s .. 31 days.
// Does not return when the PMU cuts power. Returns ESP_ERR_INVALID_STATE when
// SOFT_SLEEP latched but power stayed on for 5 s (pending sleep cancelled with
// HOST_READY), or another error when it was never latched. The board lock is
// released on every return.
esp_err_t read_pico_pmu_session_sleep(uint32_t alarm_seconds);
#ifdef __cplusplus
}
#endif
