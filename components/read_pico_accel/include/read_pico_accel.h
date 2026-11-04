#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int16_t x_mg, y_mg, z_mg;  // sensor axes, milli-g (gravity reads about 1000)
} read_pico_accel_sample_t;

// One initialization attempt per boot on the shared board I2C bus (SC7A20H at
// 0x19, 12.5 Hz low power, ±2 g). A failure is final until restart.
esp_err_t read_pico_accel_start(void);
bool read_pico_accel_ready(void);
// One sample. ESP_ERR_TIMEOUT: the board lock is busy (e.g. a refresh).
esp_err_t read_pico_accel_read(read_pico_accel_sample_t *out);
#ifdef __cplusplus
}
#endif
