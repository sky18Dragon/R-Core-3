#include "read_pico_orientation.h"

#include <stdatomic.h>

#include "config_manager.h"
#include "display_manager.h"
#include "esp_log.h"
#include "frame_orientation.h"
#include "photo_meta.h"
#include "read_pico_accel.h"
#include "utils.h"
#if CONFIG_READ_PICO_HOTSPOT
#include "read_pico_hotspot.h"
#endif

#define ORIENTATION_POLL_MS 1000

static const char *TAG = "orientation";

// Sensor axes onto the panel, measured on the board (2026-11-04) by holding the
// frame in each position: upright x≈+986 mg, turned 90° y≈-951, 180° x≈-960,
// 270° y≈+986 (z≈-210 while leaning on its stand). Down = +x, right = -y.
static const frame_axes_t s_axes = {1, -1, 0, 1};

static frame_orientation_tracker_t s_tracker;
static int64_t s_next_poll_ms;
static atomic_int s_settled = ATOMIC_VAR_INIT(0);     // from the sensor
static atomic_int s_effective = ATOMIC_VAR_INIT(0);   // after the setting
static atomic_bool s_recheck = ATOMIC_VAR_INIT(false);
static int16_t s_last[3];
static int s_last_reading = FRAME_ORIENTATION_UNKNOWN;
static bool s_have_sample;

static int effective_orientation(void)
{
    switch (config_manager_get_orient_mode()) {
    case FRAME_ORIENT_LANDSCAPE: return 0;
    case FRAME_ORIENT_PORTRAIT: return 90;
    default: return atomic_load(&s_settled);
    }
}

// The frame now stands differently: keep the picture if it still suits (turned
// half way if needed), otherwise move on to one that does -- or to the hint
// page when none does. The hotspot page is simply drawn again the new way.
static void follow_orientation(int old_orientation, int orientation)
{
    ESP_LOGI(TAG, "Showing for %d deg (was %d)", orientation, old_orientation);
#if CONFIG_READ_PICO_HOTSPOT
    if (read_pico_hotspot_on()) {
        read_pico_hotspot_show_screen();
        return;
    }
#endif
    char current[DISPLAY_IMAGE_PATH_CAPACITY];
    bool known = false;
    if (display_manager_copy_current_image(current, sizeof(current), &known) && known &&
        current[0]) {
        int up = photo_meta_lookup_up(current);
        int turn = photo_meta_turn(up, orientation);
        if (turn % 180 == 0) {
            if (turn != photo_meta_turn(up, old_orientation)) display_manager_show_image(current);
            return;
        }
    }
    trigger_image_rotation();
}

esp_err_t read_pico_orientation_init(void)
{
    frame_orientation_tracker_init(&s_tracker, FRAME_ORIENTATION_UNKNOWN);
    atomic_store(&s_effective, effective_orientation());
    return read_pico_accel_start();
}

void read_pico_orientation_setting_changed(void)
{
    atomic_store(&s_recheck, true);
}

static void settle(void)
{
    int before = atomic_load(&s_effective);
    int after = effective_orientation();
    if (after == before) return;
    atomic_store(&s_effective, after);
    follow_orientation(before, after);
}

void read_pico_orientation_poll(int64_t now_ms)
{
    if (atomic_exchange(&s_recheck, false)) settle();
    if (now_ms < s_next_poll_ms || !read_pico_accel_ready()) return;
    s_next_poll_ms = now_ms + ORIENTATION_POLL_MS;
    read_pico_accel_sample_t sample;
    if (read_pico_accel_read(&sample) != ESP_OK) return;  // busy (a refresh): next second
    s_last[0] = sample.x_mg;
    s_last[1] = sample.y_mg;
    s_last[2] = sample.z_mg;
    s_have_sample = true;
    int right = 0, down = 0;
    frame_gravity(&s_axes, s_last, &right, &down);
    s_last_reading = frame_orientation_from_gravity(right, down);
    if (frame_orientation_feed(&s_tracker, s_last_reading, now_ms)) {
        atomic_store(&s_settled, s_tracker.settled);
        ESP_LOGI(TAG, "Frame now stands at %d deg (x=%d y=%d z=%d mg)", s_tracker.settled,
                 sample.x_mg, sample.y_mg, sample.z_mg);
        settle();
    }
}

int read_pico_orientation_get(void)
{
    return atomic_load(&s_effective);
}

void read_pico_orientation_describe(cJSON *object)
{
    cJSON_AddBoolToObject(object, "sensor", read_pico_accel_ready());
    if (s_have_sample) {
        cJSON *raw = cJSON_AddObjectToObject(object, "accel_mg");
        cJSON_AddNumberToObject(raw, "x", s_last[0]);
        cJSON_AddNumberToObject(raw, "y", s_last[1]);
        cJSON_AddNumberToObject(raw, "z", s_last[2]);
    }
    if (s_last_reading == FRAME_ORIENTATION_UNKNOWN)
        cJSON_AddNullToObject(object, "reading");
    else
        cJSON_AddNumberToObject(object, "reading", s_last_reading);
    cJSON_AddNumberToObject(object, "sensed", atomic_load(&s_settled));
    cJSON_AddNumberToObject(object, "orientation", read_pico_orientation_get());
}
