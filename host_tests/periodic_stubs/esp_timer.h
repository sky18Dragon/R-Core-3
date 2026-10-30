#pragma once
#include <stdint.h>
#include "esp_err.h"
typedef void *esp_timer_handle_t;
typedef struct { void (*callback)(void *); void *arg; const char *name; } esp_timer_create_args_t;
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t esp_timer_create(const esp_timer_create_args_t *, esp_timer_handle_t *);
esp_err_t esp_timer_start_periodic(esp_timer_handle_t, uint64_t);
esp_err_t esp_timer_stop_blocking(esp_timer_handle_t, uint32_t);
esp_err_t esp_timer_delete(esp_timer_handle_t);
#ifdef __cplusplus
}
#endif
