#include "touch_gesture.h"
#include <string.h>

void touch_tracker_reset(touch_tracker_t *tracker)
{
    if (tracker) memset(tracker, 0, sizeof(*tracker));
}

bool touch_tracker_active(const touch_tracker_t *tracker)
{
    return tracker && tracker->active;
}

static bool finish(touch_tracker_t *t, touch_gesture_kind_t kind, int key, int64_t now_ms,
                   touch_gesture_t *out)
{
    *out = (touch_gesture_t) {
        .kind = kind, .key = key, .x0 = t->x0, .y0 = t->y0, .x1 = t->x, .y1 = t->y,
        .hold_ms = now_ms >= t->t0_ms ? now_ms - t->t0_ms : 0,
    };
    touch_tracker_reset(t);
    return true;
}

static touch_gesture_kind_t classify(const touch_tracker_t *t, int64_t hold_ms)
{
    int32_t dx = (int32_t) t->x - t->x0, dy = (int32_t) t->y - t->y0;
    int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    if (hold_ms <= TOUCH_SWIPE_MAX_MS) {
        if (ax >= TOUCH_SWIPE_MIN_PX && ax > ay)
            return dx < 0 ? TOUCH_GESTURE_SWIPE_X_NEG : TOUCH_GESTURE_SWIPE_X_POS;
        if (ay >= TOUCH_SWIPE_MIN_PX && ay > ax)
            return dy < 0 ? TOUCH_GESTURE_SWIPE_Y_NEG : TOUCH_GESTURE_SWIPE_Y_POS;
    }
    return t->moved ? TOUCH_GESTURE_CANCEL : TOUCH_GESTURE_TAP;
}

bool touch_tracker_feed(touch_tracker_t *t, bool touched, uint8_t count, uint16_t x, uint16_t y,
                        int64_t now_ms, touch_gesture_t *out)
{
    if (!t || !out) return false;
    bool invalid = count > 1 || (t->active && now_ms < t->last_ms);
    if (invalid) {
        if (!t->active) return false;
        return finish(t, TOUCH_GESTURE_CANCEL, -1, now_ms, out);
    }
    bool empty_slot = touched && x == 0 && y == 0;
    if (!t->active) {
        if (!touched || empty_slot) return false;
        *t = (touch_tracker_t) {.active = true, .on_key = y >= TOUCH_KEY_AREA_TOP, .x0 = x, .y0 = y,
                                .x = x, .y = y, .t0_ms = now_ms, .last_ms = now_ms};
        return false;
    }
    t->last_ms = now_ms;
    if (touched) {
        if (empty_slot) return false;
        t->x = x; t->y = y;
        int32_t dx = (int32_t) x - t->x0, dy = (int32_t) y - t->y0;
        if ((int64_t) dx * dx + (int64_t) dy * dy > (int64_t) TOUCH_SLOP_PX * TOUCH_SLOP_PX)
            t->moved = true;
        return false;
    }
    int64_t hold = now_ms - t->t0_ms;
    if (t->on_key) {
        int key = t->x0 / TOUCH_KEY_PITCH;
        if (!t->moved && key < TOUCH_KEY_COUNT) return finish(t, TOUCH_GESTURE_KEY, key, now_ms, out);
        return finish(t, TOUCH_GESTURE_CANCEL, -1, now_ms, out);
    }
    return finish(t, classify(t, hold), -1, now_ms, out);
}

touch_action_t touch_action_for(const touch_gesture_t *g, bool landscape, bool rotated_180)
{
    if (!g) return TOUCH_ACTION_NONE;
    if (g->kind == TOUCH_GESTURE_KEY) {
        if (g->key != 0 && g->key != TOUCH_KEY_COUNT - 1) return TOUCH_ACTION_NONE;
        bool first = g->key == 0;
        return first != rotated_180 ? TOUCH_ACTION_PREVIOUS : TOUCH_ACTION_NEXT;
    }
    // Assumption: landscape puts the viewer's horizontal on the native y axis
    // and portrait on native x, both increasing to the viewer's right at 0 deg.
    touch_gesture_kind_t toward_left = landscape ? TOUCH_GESTURE_SWIPE_Y_NEG : TOUCH_GESTURE_SWIPE_X_NEG;
    touch_gesture_kind_t toward_right = landscape ? TOUCH_GESTURE_SWIPE_Y_POS : TOUCH_GESTURE_SWIPE_X_POS;
    if (rotated_180) {
        touch_gesture_kind_t swap = toward_left;
        toward_left = toward_right;
        toward_right = swap;
    }
    if (g->kind == toward_left) return TOUCH_ACTION_NEXT;
    if (g->kind == toward_right) return TOUCH_ACTION_PREVIOUS;
    return TOUCH_ACTION_NONE;
}

const char *touch_gesture_name(touch_gesture_kind_t kind)
{
    static const char *const names[] = {"cancel", "tap", "key", "swipe_x_neg", "swipe_x_pos",
                                        "swipe_y_neg", "swipe_y_pos"};
    return (unsigned) kind < sizeof(names) / sizeof(names[0]) ? names[kind] : "unknown";
}

const char *touch_action_name(touch_action_t action)
{
    return action == TOUCH_ACTION_NEXT ? "next" : action == TOUCH_ACTION_PREVIOUS ? "previous" : "none";
}
