#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
    READ_PICO_BACKGROUND_SLEEP,
    READ_PICO_BACKGROUND_ROTATION,
    READ_PICO_BACKGROUND_BUTTON,
    READ_PICO_BACKGROUND_LATE_WIFI,
    READ_PICO_BACKGROUND_PERIODIC,
    READ_PICO_BACKGROUND_COUNT
} read_pico_background_role_t;

#define READ_PICO_BACKGROUND_BIT(role) (UINT32_C(1) << (role))

// Single boot only: slots and cancellation are never reset/reused. Callbacks
// must cooperate with running(), release their own resources, and RETURN.
esp_err_t read_pico_background_add(read_pico_background_role_t role, TaskFunction_t callback,
                                   void *argument, uint32_t stack_bytes, UBaseType_t priority);
// Publishes external readiness before releasing waiting callbacks. The caller
// must withdraw that readiness if publication races cancellation or fails.
esp_err_t read_pico_background_activate(uint32_t required_roles, esp_err_t (*publish)(void));
bool read_pico_background_preparing(void);
bool read_pico_background_running(void);
void read_pico_background_cancel(void);
// Bounded cooperative wait (<=5s), excluding ONLY an executing owned caller.
// ESP_OK means other callbacks returned, NOT that the idle task freed memory
// or that timer/HTTP/SDK/other application resources have been closed.
esp_err_t read_pico_background_cancel_and_wait(TickType_t timeout_ticks);
