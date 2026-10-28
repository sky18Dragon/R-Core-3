#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Portable single-contact gesture recognizer for the Read Pico CST836U panel.
// Coordinates are the panel's native (vendor portrait) frame: x across the
// short edge, y along the long edge; y >= TOUCH_KEY_AREA_TOP is the strip of
// three touch keys past the display edge. Thresholds follow the vendor UI
// (ui_kit.h / ui_menu.h, Apache-2.0); swipe length is raised for a photo frame.
#define TOUCH_KEY_AREA_TOP 1300
#define TOUCH_KEY_PITCH 160
#define TOUCH_KEY_COUNT 3
#define TOUCH_SLOP_PX 24
#define TOUCH_SWIPE_MIN_PX 120
#define TOUCH_SWIPE_MAX_MS 1500

typedef enum {
    TOUCH_GESTURE_CANCEL,
    TOUCH_GESTURE_TAP,
    TOUCH_GESTURE_KEY,          // key holds the key index 0..TOUCH_KEY_COUNT-1
    TOUCH_GESTURE_SWIPE_X_NEG,  // native x decreasing
    TOUCH_GESTURE_SWIPE_X_POS,
    TOUCH_GESTURE_SWIPE_Y_NEG,  // native y decreasing
    TOUCH_GESTURE_SWIPE_Y_POS,
} touch_gesture_kind_t;

typedef struct {
    touch_gesture_kind_t kind;
    int key;
    uint16_t x0, y0, x1, y1;
    int64_t hold_ms;
} touch_gesture_t;

typedef struct {
    bool active, on_key, moved;
    uint16_t x0, y0, x, y;
    int64_t t0_ms, last_ms;
} touch_tracker_t;

typedef enum {
    TOUCH_ACTION_NONE,
    TOUCH_ACTION_NEXT,
    TOUCH_ACTION_PREVIOUS,
} touch_action_t;

void touch_tracker_reset(touch_tracker_t *tracker);
bool touch_tracker_active(const touch_tracker_t *tracker);
// Feed one panel sample. Returns true and fills out when a contact ends
// (release, multi-touch or clock regression cancel). A pressed (0,0) sample is
// an empty driver slot, not a contact position, and is ignored.
bool touch_tracker_feed(touch_tracker_t *tracker, bool touched, uint8_t count, uint16_t x,
                        uint16_t y, int64_t now_ms, touch_gesture_t *out);
// Map a gesture to a frame action for the configured display layout.
// Swipe left (as the viewer sees it) = next, swipe right = previous; keys:
// first = previous, last = next, middle unused. The native-to-viewer axis
// mapping is a documented assumption until confirmed on the device.
touch_action_t touch_action_for(const touch_gesture_t *gesture, bool landscape, bool rotated_180);
const char *touch_gesture_name(touch_gesture_kind_t kind);
const char *touch_action_name(touch_action_t action);
#ifdef __cplusplus
}
#endif
