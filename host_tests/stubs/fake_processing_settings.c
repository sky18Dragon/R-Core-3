// Fake processing settings for host tests — only what image_processor.c
// links; both knobs are test-controllable globals.
#include <stdio.h>

#include "processing_settings.h"

scale_mode_t test_scale_mode = SCALE_MODE_COVER;
const char *test_background_color = "white";
esp_err_t test_processing_options_error = ESP_OK;
unsigned test_processing_options_reads;

esp_err_t processing_settings_copy_options(processing_options_t *out)
{
    ++test_processing_options_reads;
    if (test_processing_options_error != ESP_OK) return test_processing_options_error;
    *out = (processing_options_t){.dither_algorithm = DITHER_FLOYD_STEINBERG,
                                 .scale_mode = test_scale_mode};
    snprintf(out->background_color, sizeof(out->background_color), "%s", test_background_color);
    return ESP_OK;
}

scale_mode_t processing_settings_get_scale_mode(void)
{
    return test_scale_mode;
}

void processing_settings_get_background_color(char *out, size_t out_size)
{
    snprintf(out, out_size, "%s", test_background_color);
}
