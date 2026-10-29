#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Portable decision for the opt-in PMU soft-sleep frame mode. The frame only
// sleeps on battery (definite, not "unknown" power), with sleep enabled, when
// nothing is in progress and the awake window for this wake has passed.
#define SLEEP_AWAKE_AFTER_ALARM_MS (60 * 1000)        // short window after a timed wake
#define SLEEP_AWAKE_AFTER_KEY_MS (5 * 60 * 1000)      // longer after a button wake/boot
#define SLEEP_AWAKE_AFTER_ACTIVITY_MS (2 * 60 * 1000) // after the last web/touch activity
#define SLEEP_MIN_SECONDS 60u                          // not worth sleeping for less

typedef enum {
    SLEEP_POWER_UNKNOWN,
    SLEEP_POWER_EXTERNAL,
    SLEEP_POWER_BATTERY,
} sleep_power_t;

typedef struct {
    bool enabled;              // user setting
    sleep_power_t power;
    bool woke_by_alarm;
    int64_t uptime_ms;
    int64_t last_activity_ms;  // uptime of the last web/touch/upload activity, 0 if none
    bool busy;                 // display refresh, upload, OTA in progress
    int64_t seconds_to_next;   // until the next scheduled rotation, < 0 when none
} sleep_inputs_t;

typedef struct {
    bool sleep;
    uint32_t alarm_seconds;  // 0: no alarm (wake only by key)
} sleep_decision_t;

sleep_decision_t sleep_decide(const sleep_inputs_t *in);
#ifdef __cplusplus
}
#endif
