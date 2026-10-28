#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <stdbool.h>
#include <stdint.h>

#include "cJSON.h"
#include "esp_err.h"

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_rgb_t;

typedef struct {
    color_rgb_t black;
    color_rgb_t white;
    color_rgb_t yellow;
    color_rgb_t red;
    color_rgb_t blue;
    color_rgb_t green;
    // GC16 grayscale calibration: measured RELATIVE LUMINANCE (Y, 0..1) of full
    // black / full white, plus a mid-level gamma (1.0 = perceptually linear,
    // >1 darkens mids). The 16-level perceived ramp is derived from these
    // downstream (epaper-image-convert). Unused for color panels.
    float gray_black_y;
    float gray_white_y;
    float gray_gamma;
} color_palette_t;

esp_err_t color_palette_init(void);
esp_err_t color_palette_save(const color_palette_t *palette);
esp_err_t color_palette_load(color_palette_t *palette);
void color_palette_get_defaults(color_palette_t *palette);
char *color_palette_to_json(const color_palette_t *palette);
void color_palette_from_json(cJSON *json, color_palette_t *palette);
// Read Pico checked conversion; selected RGB components must be integral 0..255
// and calibration numbers finite/float-representable. No automatic calibration.
// Candidate/output is untouched on any error; unknown fields remain ignored.
esp_err_t color_palette_from_json_checked(cJSON *json, color_palette_t *palette);

// Read Pico: optional measured GC16 ramp, one relative luminance (Y, 0..1] per
// gray code 0..15. Stored under its own NVS key so the palette blob layout is
// unchanged; when absent the black/white/gamma model is used. When present it
// replaces that model on the device and is reported to the web converter.
#define COLOR_PALETTE_GRAY_LEVELS 16
typedef enum {
    COLOR_PALETTE_GRAY_LEVELS_KEEP = 0, // request did not mention "levels_y"
    COLOR_PALETTE_GRAY_LEVELS_CLEAR,    // "levels_y": null
    COLOR_PALETTE_GRAY_LEVELS_SET,      // "levels_y": [16 numbers]
} color_palette_gray_levels_action_t;
// Finite, 0 < y <= 1, non-decreasing, and full white brighter than full black.
bool color_palette_gray_levels_valid(const float levels[COLOR_PALETTE_GRAY_LEVELS]);
// Missing namespace/key: ESP_OK with *present=false. Other read, size or value
// errors are returned and leave the outputs untouched.
esp_err_t color_palette_load_gray_levels(float levels[COLOR_PALETTE_GRAY_LEVELS], bool *present);
// NULL erases the key (already missing counts as success).
esp_err_t color_palette_save_gray_levels(const float levels[COLOR_PALETTE_GRAY_LEVELS]);
// Parse "levels_y" from a palette request. Invalid input returns
// ESP_ERR_INVALID_ARG and leaves the outputs untouched.
esp_err_t color_palette_gray_levels_from_json(cJSON *json, color_palette_gray_levels_action_t *action,
                                              float levels[COLOR_PALETTE_GRAY_LEVELS]);
// color_palette_to_json() plus "levels_y" when `levels` is not NULL.
char *color_palette_to_json_with_levels(const color_palette_t *palette,
                                        const float levels[COLOR_PALETTE_GRAY_LEVELS]);

#endif
