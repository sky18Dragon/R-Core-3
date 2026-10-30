#pragma once
#include <stdint.h>
#include "cJSON.h"
#ifdef __cplusplus
extern "C" {
#endif
// Local-frame touch navigation: call from one task about every 50 ms. Starts
// the panel lazily, turns gestures into next/previous image, keeps only the
// latest request while the display is busy, and resets the rotation timer
// after a manual change. Blocks the caller for one refresh when acting.
void read_pico_touch_nav_poll(int64_t now_ms);
// Recent gestures (newest last) with raw panel coordinates, for calibrating
// the swipe direction on the device. Adds "ready" and "events" to object.
void read_pico_touch_nav_describe(cJSON *object);
#ifdef __cplusplus
}
#endif
