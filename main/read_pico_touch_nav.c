#include "read_pico_touch_nav.h"

#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "config_manager.h"
#include "display_manager.h"
#include "image_history.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "power_manager.h"
#include "read_pico_touch.h"
#include "touch_gesture.h"
#include "utils.h"

#define TOUCH_NAV_LOG_DEPTH 16
#define TOUCH_NAV_START_RETRY_MS 5000
#define TOUCH_NAV_PREVIOUS_ATTEMPTS 3

typedef struct {
    int64_t at_ms;
    touch_gesture_t gesture;
    touch_action_t action;
    esp_err_t result;
} touch_nav_event_t;

static const char *TAG = "touch_nav";
static touch_tracker_t s_tracker;
static touch_action_t s_pending = TOUCH_ACTION_NONE;
static int64_t s_next_start_ms;
static touch_nav_event_t s_log[TOUCH_NAV_LOG_DEPTH];
static unsigned s_log_count, s_log_next;
static portMUX_TYPE s_log_lock = portMUX_INITIALIZER_UNLOCKED;

static void record(int64_t now_ms, const touch_gesture_t *gesture, touch_action_t action, esp_err_t result)
{
    taskENTER_CRITICAL(&s_log_lock);
    s_log[s_log_next] = (touch_nav_event_t) {now_ms, *gesture, action, result};
    s_log_next = (s_log_next + 1) % TOUCH_NAV_LOG_DEPTH;
    if (s_log_count < TOUCH_NAV_LOG_DEPTH) ++s_log_count;
    taskEXIT_CRITICAL(&s_log_lock);
}

static esp_err_t show_previous(void)
{
    char path[IMAGE_HISTORY_PATH_MAX];
    for (int i = 0; i < TOUCH_NAV_PREVIOUS_ATTEMPTS; ++i) {
        if (!display_manager_take_previous(path, sizeof(path)))
            return ESP_ERR_NOT_FOUND;
        esp_err_t err = display_manager_show_image(path);
        if (err == ESP_OK || err == ESP_ERR_NOT_FINISHED)
            return err;
        ESP_LOGW(TAG, "Previous image unavailable (%s): %s", esp_err_to_name(err), path);
        display_manager_forget_history(path);
    }
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t run(touch_action_t action)
{
    esp_err_t err = action == TOUCH_ACTION_NEXT ? trigger_image_rotation() : show_previous();
    // A manual change starts a fresh rotation interval either way.
    power_manager_reset_rotate_timer();
    ESP_LOGI(TAG, "Touch %s: %s", touch_action_name(action), esp_err_to_name(err));
    return err;
}

void read_pico_touch_nav_poll(int64_t now_ms)
{
    if (!read_pico_touch_ready()) {
        if (now_ms < s_next_start_ms)
            return;
        s_next_start_ms = now_ms + TOUCH_NAV_START_RETRY_MS;
        if (read_pico_touch_start() != ESP_OK)
            return;  // busy: retried later; a definite failure stays disabled
    }
    if (s_pending != TOUCH_ACTION_NONE && !display_manager_is_busy()) {
        touch_action_t action = s_pending;
        s_pending = TOUCH_ACTION_NONE;
        touch_gesture_t none = {.kind = TOUCH_GESTURE_CANCEL, .key = -1};
        record(now_ms, &none, action, run(action));
        touch_tracker_reset(&s_tracker);  // ignore contact that spanned the refresh
        return;
    }
    read_pico_touch_sample_t sample;
    esp_err_t err = read_pico_touch_read(&sample, touch_tracker_active(&s_tracker));
    if (err != ESP_OK)
        return;  // board busy (refresh) or read error: keep tracking state
    touch_gesture_t gesture;
    if (!touch_tracker_feed(&s_tracker, sample.touched, sample.count, sample.x, sample.y, now_ms,
                            &gesture))
        return;
    bool landscape = config_manager_get_display_orientation() == DISPLAY_ORIENTATION_LANDSCAPE;
    bool rotated = config_manager_get_display_rotation_deg() % 360 == 180;
    touch_action_t action = touch_action_for(&gesture, landscape, rotated);
    ESP_LOGI(TAG, "Gesture %s key=%d (%u,%u)->(%u,%u) %lldms -> %s", touch_gesture_name(gesture.kind),
             gesture.key, gesture.x0, gesture.y0, gesture.x1, gesture.y1, (long long) gesture.hold_ms,
             touch_action_name(action));
    if (action == TOUCH_ACTION_NONE) {
        record(now_ms, &gesture, action, ESP_OK);
        return;
    }
    if (display_manager_is_busy()) {
        s_pending = action;  // only the latest request survives a refresh
        record(now_ms, &gesture, action, ESP_ERR_TIMEOUT);
        return;
    }
    record(now_ms, &gesture, action, run(action));
}

void read_pico_touch_nav_describe(cJSON *object)
{
    touch_nav_event_t copy[TOUCH_NAV_LOG_DEPTH];
    unsigned count, next;
    taskENTER_CRITICAL(&s_log_lock);
    memcpy(copy, s_log, sizeof(copy));
    count = s_log_count;
    next = s_log_next;
    taskEXIT_CRITICAL(&s_log_lock);
    cJSON_AddBoolToObject(object, "ready", read_pico_touch_ready());
    cJSON *events = cJSON_AddArrayToObject(object, "events");
    if (!events)
        return;
    for (unsigned i = 0; i < count; ++i) {
        const touch_nav_event_t *e = &copy[(next + TOUCH_NAV_LOG_DEPTH - count + i) % TOUCH_NAV_LOG_DEPTH];
        cJSON *item = cJSON_CreateObject();
        if (!item)
            return;
        cJSON_AddNumberToObject(item, "at_ms", (double) e->at_ms);
        cJSON_AddStringToObject(item, "gesture", touch_gesture_name(e->gesture.kind));
        cJSON_AddNumberToObject(item, "key", e->gesture.key);
        cJSON_AddNumberToObject(item, "x0", e->gesture.x0);
        cJSON_AddNumberToObject(item, "y0", e->gesture.y0);
        cJSON_AddNumberToObject(item, "x1", e->gesture.x1);
        cJSON_AddNumberToObject(item, "y1", e->gesture.y1);
        cJSON_AddNumberToObject(item, "hold_ms", (double) e->gesture.hold_ms);
        cJSON_AddStringToObject(item, "action", touch_action_name(e->action));
        cJSON_AddStringToObject(item, "result", esp_err_to_name(e->result));
        cJSON_AddItemToArray(events, item);
    }
}
