#include "read_pico_photo_bridge.h"

#include <stdio.h>

#include "board_hal.h"
#include "config_manager.h"
#include "display_manager.h"
#include "esp_heap_caps.h"
#include "processing_settings.h"
#include "read_pico_photo_frame.h"

#if !CONFIG_READ_PICO_PHOTO_VALIDATION
#error "Diagnostic sink must never replace the product display_manager/settings"
#endif

static photo_frame_t frame;
static bool active, ready;

// Fixed, RAM-only diagnostic settings; the real product implementations are not linked here.
esp_err_t processing_settings_copy_options(processing_options_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = (processing_options_t){.dither_algorithm = DITHER_FLOYD_STEINBERG,
                                 .scale_mode = SCALE_MODE_COVER, .background_color = "white"};
    return ESP_OK;
}

display_orientation_t config_manager_get_display_orientation(void)
{
    return DISPLAY_ORIENTATION_PORTRAIT;
}
scale_mode_t processing_settings_get_scale_mode(void)
{
    return SCALE_MODE_COVER;
}
void processing_settings_get_background_color(char *out, size_t size)
{
    if (out && size)
        snprintf(out, size, "white");
}

esp_err_t display_manager_begin_rgb_stream(void)
{
    if (frame.pixels || active || ready)
        return ESP_ERR_INVALID_STATE;
    uint8_t *pixels = heap_caps_malloc(READ_PICO_FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!pixels)
        return ESP_ERR_NO_MEM;
    if (!photo_frame_begin(&frame, pixels, READ_PICO_FRAME_BYTES,
                           BOARD_HAL_DISPLAY_ROTATION_DEG == 180)) {
        heap_caps_free(pixels);
        return ESP_FAIL;
    }
    active = true;
    return ESP_OK;
}
esp_err_t display_manager_push_rgb_row(int y, const uint8_t *rgb, int width)
{
    return active && photo_frame_push(&frame, false, y, rgb, width) ? ESP_OK
                                                                    : ESP_ERR_INVALID_STATE;
}
esp_err_t display_manager_push_rgb_column(int x, const uint8_t *rgb, int height)
{
    return active && photo_frame_push(&frame, true, x, rgb, height) ? ESP_OK
                                                                    : ESP_ERR_INVALID_STATE;
}
esp_err_t display_manager_end_rgb_stream(bool show, const display_publish_t *pub)
{
    if (!active)
        return ESP_ERR_INVALID_STATE;
    active = false;
    ready = show && !pub && photo_frame_complete(&frame);
    if (ready)
        return ESP_OK;
    read_pico_photo_bridge_release();
    return show ? ESP_ERR_INVALID_STATE : ESP_OK;
}
const uint8_t *read_pico_photo_bridge_frame(void)
{
    return ready && !active ? frame.pixels : NULL;
}
void read_pico_photo_bridge_release(void)
{
    heap_caps_free(frame.pixels);
    frame = (photo_frame_t) {0};
    active = ready = false;
}
