#pragma once
#include <stdint.h>
#include "cJSON.h"
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
// Local frame: which way the frame stands, from the SC7A20H accelerometer
// (frame_orientation.h). The button task polls; others read the settled value.
esp_err_t read_pico_orientation_init(void);
void read_pico_orientation_poll(int64_t now_ms);
// The orientation photos are chosen and drawn for: 0 / 90 / 180 / 270 clockwise
// from normal landscape, from the sensor (0 until known) or the fixed setting.
int read_pico_orientation_get(void);
// The orientation setting changed (any task); the button task applies it.
void read_pico_orientation_setting_changed(void);
// For GET /api/orientation: sensor reading, the position it means, the settled one.
void read_pico_orientation_describe(cJSON *object);
#ifdef __cplusplus
}
#endif
