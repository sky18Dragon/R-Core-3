#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
// Opt-in battery sleep for the local frame (CONFIG_READ_PICO_PMU_SLEEP).
// The user setting is stored separately from the upstream deep_sleep_enabled,
// which stays off so the ESP deep-sleep path never runs on this board.

// After board init: HOST_READY and record why the PMU powered us on.
void frame_sleep_boot(void);
bool frame_sleep_woke_by_alarm(void);
// Any web/touch/upload activity keeps the frame awake a while longer.
void frame_sleep_note_activity(void);
// Call about once a second from a task that may block for a moment.
void frame_sleep_tick(int64_t now_ms);
bool frame_sleep_enabled(void);
esp_err_t frame_sleep_set_enabled(bool enabled);
// "enabled", supply, battery, PMU states, last decision.
void frame_sleep_describe(cJSON *object);
#ifdef __cplusplus
}
#endif
