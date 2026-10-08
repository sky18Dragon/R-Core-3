#include "color_palette.h"

#include <string.h>
#include <math.h>
#include <float.h>

#include "board_hal.h"
#include "cJSON.h"
#include "config.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "color_palette";
static const char *NVS_KEY_PALETTE = "palette";

void color_palette_get_defaults(color_palette_t *palette)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    memset(palette, 0, sizeof(*palette)); // Deterministic blob padding; numeric defaults unchanged.
#endif
    palette->black = (color_rgb_t){2, 2, 2};
    palette->white = (color_rgb_t){190, 200, 200};
    palette->yellow = (color_rgb_t){205, 202, 0};
    palette->red = (color_rgb_t){135, 19, 0};
    palette->blue = (color_rgb_t){5, 64, 158};
    palette->green = (color_rgb_t){39, 102, 60};
    // GC16 grayscale calibration (Y, 0..1; gamma shapes mid-levels), panel-
    // measured + calibratable. Defaults are the measured ED103TC2 calibration.
    palette->gray_black_y = 0.009f;
    palette->gray_white_y = 0.65f;
    palette->gray_gamma = 1.42f;
}

esp_err_t color_palette_init(void)
{
    ESP_LOGI(TAG, "Initializing color palette");
    return ESP_OK;
}

esp_err_t color_palette_save(const color_palette_t *palette)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!palette || !isfinite(palette->gray_black_y) || !isfinite(palette->gray_white_y) ||
        !isfinite(palette->gray_gamma)) return ESP_ERR_INVALID_ARG;
#endif
    nvs_handle_t handle;
    esp_err_t err;

    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, NVS_KEY_PALETTE, palette, sizeof(color_palette_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Color palette saved to NVS");
        } else {
            ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "Failed to write palette to NVS: %s", esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}

esp_err_t color_palette_load(color_palette_t *palette)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!palette) return ESP_ERR_INVALID_ARG;
    color_palette_t candidate;
    color_palette_get_defaults(&candidate);
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) { *palette = candidate; return ESP_OK; }
    if (err != ESP_OK) return err;
    size_t size = sizeof(candidate);
    err = nvs_get_blob(handle, NVS_KEY_PALETTE, &candidate, &size);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        color_palette_get_defaults(&candidate);
        err = ESP_OK;
    } else if (err == ESP_OK && size != sizeof(candidate)) err = ESP_ERR_INVALID_SIZE;
    if (err == ESP_OK && (!isfinite(candidate.gray_black_y) || !isfinite(candidate.gray_white_y) ||
                         !isfinite(candidate.gray_gamma))) err = ESP_ERR_INVALID_RESPONSE;
    if (err == ESP_OK) *palette = candidate;
    return err;
#else
    nvs_handle_t handle;
    esp_err_t err;

    // Initialize with defaults first
    color_palette_get_defaults(palette);

    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for reading: %s, using defaults", esp_err_to_name(err));
        return ESP_OK;
    }

    size_t required_size = sizeof(color_palette_t);
    err = nvs_get_blob(handle, NVS_KEY_PALETTE, palette, &required_size);

    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Color palette not found in NVS, using defaults");
        color_palette_get_defaults(palette);
    } else {
        ESP_LOGI(TAG, "Color palette loaded from NVS");
    }

    return ESP_OK;
#endif
}

static void color_from_json(cJSON *obj, color_rgb_t *color)
{
    cJSON *component;
    if ((component = cJSON_GetObjectItem(obj, "r")) && cJSON_IsNumber(component))
        color->r = (uint8_t) component->valueint;
    if ((component = cJSON_GetObjectItem(obj, "g")) && cJSON_IsNumber(component))
        color->g = (uint8_t) component->valueint;
    if ((component = cJSON_GetObjectItem(obj, "b")) && cJSON_IsNumber(component))
        color->b = (uint8_t) component->valueint;
}

void color_palette_from_json(cJSON *json, color_palette_t *palette)
{
    cJSON *color;
    if ((color = cJSON_GetObjectItem(json, "black")))
        color_from_json(color, &palette->black);
    if ((color = cJSON_GetObjectItem(json, "white")))
        color_from_json(color, &palette->white);
    if ((color = cJSON_GetObjectItem(json, "yellow")))
        color_from_json(color, &palette->yellow);
    if ((color = cJSON_GetObjectItem(json, "red")))
        color_from_json(color, &palette->red);
    if ((color = cJSON_GetObjectItem(json, "blue")))
        color_from_json(color, &palette->blue);
    if ((color = cJSON_GetObjectItem(json, "green")))
        color_from_json(color, &palette->green);

    // GC16 grayscale calibration endpoints (relative luminance Y, 0..1).
    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "black_y")) && cJSON_IsNumber(item))
        palette->gray_black_y = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "white_y")) && cJSON_IsNumber(item))
        palette->gray_white_y = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "gamma")) && cJSON_IsNumber(item))
        palette->gray_gamma = (float) item->valuedouble;
}

static cJSON *color_to_json(const color_rgb_t *color)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj) {
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (!cJSON_AddNumberToObject(obj, "r", color->r) ||
            !cJSON_AddNumberToObject(obj, "g", color->g) ||
            !cJSON_AddNumberToObject(obj, "b", color->b)) {
            cJSON_Delete(obj); return NULL;
        }
#else
        cJSON_AddNumberToObject(obj, "r", color->r);
        cJSON_AddNumberToObject(obj, "g", color->g);
        cJSON_AddNumberToObject(obj, "b", color->b);
#endif
    }
    return obj;
}

char *color_palette_to_json(const color_palette_t *palette)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!palette || !isfinite(palette->gray_black_y) || !isfinite(palette->gray_white_y) ||
        !isfinite(palette->gray_gamma)) return NULL;
#endif
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }

    if (strncmp(BOARD_HAL_DISPLAY_TYPE, "gc", 2) == 0) {
        // Grayscale (GC16): no per-color calibration. Report the two measured
        // luminance endpoints (Y of full black/white); the 16-level perceived
        // ramp is derived from these by epaper-image-convert.
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (!cJSON_AddNumberToObject(json, "black_y", palette->gray_black_y) ||
            !cJSON_AddNumberToObject(json, "white_y", palette->gray_white_y) ||
            !cJSON_AddNumberToObject(json, "gamma", palette->gray_gamma)) {
            cJSON_Delete(json); return NULL;
        }
#else
        cJSON_AddNumberToObject(json, "black_y", palette->gray_black_y);
        cJSON_AddNumberToObject(json, "white_y", palette->gray_white_y);
        cJSON_AddNumberToObject(json, "gamma", palette->gray_gamma);
#endif
    } else {
#if CONFIG_BOARD_DRIVER_READ_PICO
        const char *names[] = {"black", "white", "yellow", "red", "blue", "green"};
        const color_rgb_t *colors[] = {&palette->black, &palette->white, &palette->yellow,
                                     &palette->red, &palette->blue, &palette->green};
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
            cJSON *color = color_to_json(colors[i]);
            if (!color || !cJSON_AddItemToObject(json, names[i], color)) {
                cJSON_Delete(color); cJSON_Delete(json); return NULL;
            }
        }
#else
        cJSON_AddItemToObject(json, "black", color_to_json(&palette->black));
        cJSON_AddItemToObject(json, "white", color_to_json(&palette->white));
        cJSON_AddItemToObject(json, "yellow", color_to_json(&palette->yellow));
        cJSON_AddItemToObject(json, "red", color_to_json(&palette->red));
        cJSON_AddItemToObject(json, "blue", color_to_json(&palette->blue));
        cJSON_AddItemToObject(json, "green", color_to_json(&palette->green));
#endif
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    return json_str;
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t color_palette_from_json_checked(cJSON *json, color_palette_t *palette)
{
    if (!palette || !cJSON_IsObject(json)) return ESP_ERR_INVALID_ARG;
    color_palette_t candidate = *palette;
    const char *names[] = {"black", "white", "yellow", "red", "blue", "green"};
    color_rgb_t *colors[] = {&candidate.black, &candidate.white, &candidate.yellow,
                            &candidate.red, &candidate.blue, &candidate.green};
    const char *components[] = {"r", "g", "b"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        cJSON *color = cJSON_GetObjectItem(json, names[i]);
        if (!color) continue;
        if (!cJSON_IsObject(color)) return ESP_ERR_INVALID_ARG;
        uint8_t *values[] = {&colors[i]->r, &colors[i]->g, &colors[i]->b};
        for (size_t j = 0; j < 3; ++j) {
            cJSON *item = cJSON_GetObjectItem(color, components[j]);
            if (!item) continue;
            double number = item->valuedouble;
            if (!cJSON_IsNumber(item) || !isfinite(number) || number < 0 || number > 255 || trunc(number) != number)
                return ESP_ERR_INVALID_ARG;
            *values[j] = (uint8_t)number;
        }
    }
    const char *calibrations[] = {"black_y", "white_y", "gamma"};
    float *values[] = {&candidate.gray_black_y, &candidate.gray_white_y, &candidate.gray_gamma};
    for (size_t i = 0; i < 3; ++i) {
        cJSON *item = cJSON_GetObjectItem(json, calibrations[i]);
        if (!item) continue;
        if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || fabs(item->valuedouble) > FLT_MAX)
            return ESP_ERR_INVALID_ARG;
        *values[i] = (float)item->valuedouble;
    }
    *palette = candidate;
    return ESP_OK;
}
#endif
