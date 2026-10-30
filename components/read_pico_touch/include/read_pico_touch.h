#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    bool touched;
    uint8_t count;
    uint16_t x, y;  // panel native coordinates (see touch_gesture.h)
} read_pico_touch_sample_t;

// One initialization attempt per boot on the shared board I2C bus (INT on
// GPIO43). A failure is final until restart; TP_RST is never pulsed here.
esp_err_t read_pico_touch_start(void);
bool read_pico_touch_ready(void);
// Read one sample (polled; INT is not used). ESP_ERR_TIMEOUT: the board lock
// is busy (e.g. a refresh), no sample taken. tracking is currently unused.
esp_err_t read_pico_touch_read(read_pico_touch_sample_t *out, bool tracking);
#ifdef __cplusplus
}
#endif
