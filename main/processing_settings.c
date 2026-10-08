#include "processing_settings.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "cJSON.h"
#include "config.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include "config_apply.h"
#endif

static const char *TAG = "processing_settings";

#define NVS_PROC_EXPOSURE_KEY "proc_exp"
#define NVS_PROC_SATURATION_KEY "proc_sat"
#define NVS_PROC_TONE_MODE_KEY "proc_tone"
#define NVS_PROC_CONTRAST_KEY "proc_cont"
#define NVS_PROC_STRENGTH_KEY "proc_str"
#define NVS_PROC_SHADOW_KEY "proc_shad"
#define NVS_PROC_HIGHLIGHT_KEY "proc_high"
#define NVS_PROC_MIDPOINT_KEY "proc_mid"
#define NVS_PROC_COLOR_METHOD_KEY "proc_col"
#define NVS_PROC_COMPRESS_DR_KEY "proc_cdr"
#define NVS_PROC_DITHER_ALGO_KEY "proc_dith"
#define NVS_PROC_SCALE_MODE_KEY "proc_smode"
#define NVS_PROC_BG_COLOR_KEY "proc_bgcol"

void processing_settings_get_defaults(processing_settings_t *settings)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    memset(settings, 0, sizeof(*settings)); // Terminate every fixed text slot, including parser truncation limits.
#endif
    settings->exposure = 1.0f;
    settings->saturation = 1.0f;
    strncpy(settings->tone_mode, "contrast", sizeof(settings->tone_mode) - 1);
    settings->contrast = 1.0f;
    settings->strength = 0.5f;
    settings->shadow_boost = 0.0f;
    settings->highlight_compress = 0.0f;
    settings->midpoint = 0.5f;
    strncpy(settings->color_method, "rgb", sizeof(settings->color_method) - 1);
    strncpy(settings->dither_algorithm, "floyd-steinberg", sizeof(settings->dither_algorithm) - 1);
    settings->compress_dynamic_range = true;
    strncpy(settings->scale_mode, "cover", sizeof(settings->scale_mode) - 1);
    strncpy(settings->background_color, "white", sizeof(settings->background_color) - 1);
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t copy_options(void *context)
{
    processing_settings_t settings;
    esp_err_t err = processing_settings_load(&settings);
    if (err != ESP_OK) return err;
    processing_options_t candidate = {0};
    if (strcmp(settings.dither_algorithm, "floyd-steinberg") == 0)
        candidate.dither_algorithm = DITHER_FLOYD_STEINBERG;
    else if (strcmp(settings.dither_algorithm, "stucki") == 0)
        candidate.dither_algorithm = DITHER_STUCKI;
    else if (strcmp(settings.dither_algorithm, "burkes") == 0)
        candidate.dither_algorithm = DITHER_BURKES;
    else if (strcmp(settings.dither_algorithm, "sierra") == 0)
        candidate.dither_algorithm = DITHER_SIERRA;
    else return ESP_ERR_INVALID_ARG;
    if (strcmp(settings.scale_mode, "cover") == 0)
        candidate.scale_mode = SCALE_MODE_COVER;
    else if (strcmp(settings.scale_mode, "fit") == 0)
        candidate.scale_mode = SCALE_MODE_FIT;
    else return ESP_ERR_INVALID_ARG;
    if (strcmp(settings.background_color, "white") != 0 &&
        strcmp(settings.background_color, "black") != 0) return ESP_ERR_INVALID_ARG;
    memcpy(candidate.background_color, settings.background_color, sizeof(candidate.background_color));
    *(processing_options_t *)context = candidate;
    return ESP_OK;
}

esp_err_t processing_settings_copy_options(processing_options_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    return config_apply_run(copy_options, out);
}
#endif

dither_algorithm_t processing_settings_get_dithering_algorithm(void)
{
    processing_settings_t settings;
    if (processing_settings_load(&settings) != ESP_OK) {
        processing_settings_get_defaults(&settings);
    }

    // Parse dithering algorithm string to enum
    if (strcmp(settings.dither_algorithm, "stucki") == 0) {
        return DITHER_STUCKI;
    } else if (strcmp(settings.dither_algorithm, "burkes") == 0) {
        return DITHER_BURKES;
    } else if (strcmp(settings.dither_algorithm, "sierra") == 0) {
        return DITHER_SIERRA;
    }

    return DITHER_FLOYD_STEINBERG;  // default
}

scale_mode_t processing_settings_get_scale_mode(void)
{
    processing_settings_t settings;
    if (processing_settings_load(&settings) != ESP_OK) {
        processing_settings_get_defaults(&settings);
    }

    if (strcmp(settings.scale_mode, "fit") == 0) {
        return SCALE_MODE_FIT;
    }
    return SCALE_MODE_COVER;  // default
}

void processing_settings_get_background_color(char *out, size_t out_size)
{
    processing_settings_t settings;
    if (processing_settings_load(&settings) != ESP_OK) {
        processing_settings_get_defaults(&settings);
    }
    snprintf(out, out_size, "%s", settings.background_color);
}

esp_err_t processing_settings_init(void)
{
    ESP_LOGI(TAG, "Processing settings initialized");
    return ESP_OK;
}

esp_err_t processing_settings_save(const processing_settings_t *settings)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!settings || !isfinite(settings->exposure) || !isfinite(settings->saturation) ||
        !isfinite(settings->contrast) || !isfinite(settings->strength) ||
        !isfinite(settings->shadow_boost) || !isfinite(settings->highlight_compress) ||
        !isfinite(settings->midpoint)) return ESP_ERR_INVALID_ARG;
    if (!memchr(settings->tone_mode, 0, sizeof(settings->tone_mode)) ||
        !memchr(settings->color_method, 0, sizeof(settings->color_method)) ||
        !memchr(settings->dither_algorithm, 0, sizeof(settings->dither_algorithm)) ||
        !memchr(settings->scale_mode, 0, sizeof(settings->scale_mode)) ||
        !memchr(settings->background_color, 0, sizeof(settings->background_color))) return ESP_ERR_INVALID_SIZE;
#endif
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }

    // Save float values as uint32_t to avoid precision issues
    uint32_t exp_bits, sat_bits, cont_bits, str_bits, shad_bits, high_bits, mid_bits;
    memcpy(&exp_bits, &settings->exposure, sizeof(float));
    memcpy(&sat_bits, &settings->saturation, sizeof(float));
    memcpy(&cont_bits, &settings->contrast, sizeof(float));
    memcpy(&str_bits, &settings->strength, sizeof(float));
    memcpy(&shad_bits, &settings->shadow_boost, sizeof(float));
    memcpy(&high_bits, &settings->highlight_compress, sizeof(float));
    memcpy(&mid_bits, &settings->midpoint, sizeof(float));

#if CONFIG_BOARD_DRIVER_READ_PICO
    // Preserve the upstream field/key order; stop on the first unconfirmed
    // set, never commit or report success after an earlier field error.
#define SAVE_FIELD(call) do { err = (call); if (err != ESP_OK) goto close; } while (0)
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_EXPOSURE_KEY, exp_bits));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_SATURATION_KEY, sat_bits));
    SAVE_FIELD(nvs_set_str(nvs_handle, NVS_PROC_TONE_MODE_KEY, settings->tone_mode));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_CONTRAST_KEY, cont_bits));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_STRENGTH_KEY, str_bits));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_SHADOW_KEY, shad_bits));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_HIGHLIGHT_KEY, high_bits));
    SAVE_FIELD(nvs_set_u32(nvs_handle, NVS_PROC_MIDPOINT_KEY, mid_bits));
    SAVE_FIELD(nvs_set_str(nvs_handle, NVS_PROC_COLOR_METHOD_KEY, settings->color_method));
    SAVE_FIELD(nvs_set_u8(nvs_handle, NVS_PROC_COMPRESS_DR_KEY, settings->compress_dynamic_range ? 1 : 0));
    SAVE_FIELD(nvs_set_str(nvs_handle, NVS_PROC_DITHER_ALGO_KEY, settings->dither_algorithm));
    SAVE_FIELD(nvs_set_str(nvs_handle, NVS_PROC_SCALE_MODE_KEY, settings->scale_mode));
    SAVE_FIELD(nvs_set_str(nvs_handle, NVS_PROC_BG_COLOR_KEY, settings->background_color));
#undef SAVE_FIELD
#else
    nvs_set_u32(nvs_handle, NVS_PROC_EXPOSURE_KEY, exp_bits);
    nvs_set_u32(nvs_handle, NVS_PROC_SATURATION_KEY, sat_bits);
    nvs_set_str(nvs_handle, NVS_PROC_TONE_MODE_KEY, settings->tone_mode);
    nvs_set_u32(nvs_handle, NVS_PROC_CONTRAST_KEY, cont_bits);
    nvs_set_u32(nvs_handle, NVS_PROC_STRENGTH_KEY, str_bits);
    nvs_set_u32(nvs_handle, NVS_PROC_SHADOW_KEY, shad_bits);
    nvs_set_u32(nvs_handle, NVS_PROC_HIGHLIGHT_KEY, high_bits);
    nvs_set_u32(nvs_handle, NVS_PROC_MIDPOINT_KEY, mid_bits);
    nvs_set_str(nvs_handle, NVS_PROC_COLOR_METHOD_KEY, settings->color_method);
    nvs_set_u8(nvs_handle, NVS_PROC_COMPRESS_DR_KEY, settings->compress_dynamic_range ? 1 : 0);
    nvs_set_str(nvs_handle, NVS_PROC_DITHER_ALGO_KEY, settings->dither_algorithm);
    nvs_set_str(nvs_handle, NVS_PROC_SCALE_MODE_KEY, settings->scale_mode);
    nvs_set_str(nvs_handle, NVS_PROC_BG_COLOR_KEY, settings->background_color);
#endif

    err = nvs_commit(nvs_handle);
#if CONFIG_BOARD_DRIVER_READ_PICO
close:
#endif
    nvs_close(nvs_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Processing settings saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t processing_settings_load(processing_settings_t *settings)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!settings) return ESP_ERR_INVALID_ARG;
    processing_settings_t candidate;
    processing_settings_get_defaults(&candidate);
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) { *settings = candidate; return ESP_OK; }
    if (err != ESP_OK) return err;
    struct { const char *key; float *value; } numbers[] = {
        {NVS_PROC_EXPOSURE_KEY, &candidate.exposure}, {NVS_PROC_SATURATION_KEY, &candidate.saturation},
        {NVS_PROC_CONTRAST_KEY, &candidate.contrast}, {NVS_PROC_STRENGTH_KEY, &candidate.strength},
        {NVS_PROC_SHADOW_KEY, &candidate.shadow_boost}, {NVS_PROC_HIGHLIGHT_KEY, &candidate.highlight_compress},
        {NVS_PROC_MIDPOINT_KEY, &candidate.midpoint}};
    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
        uint32_t bits = 0;
        err = nvs_get_u32(handle, numbers[i].key, &bits);
        if (err == ESP_ERR_NVS_NOT_FOUND) continue;
        if (err != ESP_OK) goto close;
        float value;
        memcpy(&value, &bits, sizeof(value));
        if (!isfinite(value)) { err = ESP_ERR_INVALID_RESPONSE; goto close; }
        *numbers[i].value = value;
    }
    struct { const char *key; char *value; size_t capacity; } strings[] = {
        {NVS_PROC_TONE_MODE_KEY, candidate.tone_mode, sizeof(candidate.tone_mode)},
        {NVS_PROC_COLOR_METHOD_KEY, candidate.color_method, sizeof(candidate.color_method)},
        {NVS_PROC_DITHER_ALGO_KEY, candidate.dither_algorithm, sizeof(candidate.dither_algorithm)},
        {NVS_PROC_SCALE_MODE_KEY, candidate.scale_mode, sizeof(candidate.scale_mode)},
        {NVS_PROC_BG_COLOR_KEY, candidate.background_color, sizeof(candidate.background_color)}};
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
        char value[20] = {0}; // Largest existing fixed text slot.
        if (strings[i].capacity > sizeof(value)) { err = ESP_ERR_INVALID_SIZE; goto close; }
        size_t length = strings[i].capacity;
        err = nvs_get_str(handle, strings[i].key, value, &length);
        if (err == ESP_ERR_NVS_NOT_FOUND) continue;
        if (err != ESP_OK) goto close;
        if (!length || length > strings[i].capacity || value[length - 1] != '\0' ||
            strnlen(value, length) != length - 1) { err = ESP_ERR_INVALID_RESPONSE; goto close; }
        memcpy(strings[i].value, value, length);
    }
    uint8_t compress = 0;
    err = nvs_get_u8(handle, NVS_PROC_COMPRESS_DR_KEY, &compress);
    if (err == ESP_ERR_NVS_NOT_FOUND) err = ESP_OK;
    else if (err == ESP_OK) {
        if (compress > 1) err = ESP_ERR_INVALID_RESPONSE;
        else candidate.compress_dynamic_range = compress != 0;
    }
close:
    nvs_close(handle);
    if (err == ESP_OK) *settings = candidate;
    return err;
#else
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for reading, using defaults: %s", esp_err_to_name(err));
        processing_settings_get_defaults(settings);
        return err;
    }

    // Load with defaults as fallback
    processing_settings_get_defaults(settings);

    uint32_t exp_bits, sat_bits, cont_bits, str_bits, shad_bits, high_bits, mid_bits;

    if (nvs_get_u32(nvs_handle, NVS_PROC_EXPOSURE_KEY, &exp_bits) == ESP_OK) {
        memcpy(&settings->exposure, &exp_bits, sizeof(float));
    }
    if (nvs_get_u32(nvs_handle, NVS_PROC_SATURATION_KEY, &sat_bits) == ESP_OK) {
        memcpy(&settings->saturation, &sat_bits, sizeof(float));
    }

    size_t len = sizeof(settings->tone_mode);
    nvs_get_str(nvs_handle, NVS_PROC_TONE_MODE_KEY, settings->tone_mode, &len);

    if (nvs_get_u32(nvs_handle, NVS_PROC_CONTRAST_KEY, &cont_bits) == ESP_OK) {
        memcpy(&settings->contrast, &cont_bits, sizeof(float));
    }
    if (nvs_get_u32(nvs_handle, NVS_PROC_STRENGTH_KEY, &str_bits) == ESP_OK) {
        memcpy(&settings->strength, &str_bits, sizeof(float));
    }
    if (nvs_get_u32(nvs_handle, NVS_PROC_SHADOW_KEY, &shad_bits) == ESP_OK) {
        memcpy(&settings->shadow_boost, &shad_bits, sizeof(float));
    }
    if (nvs_get_u32(nvs_handle, NVS_PROC_HIGHLIGHT_KEY, &high_bits) == ESP_OK) {
        memcpy(&settings->highlight_compress, &high_bits, sizeof(float));
    }
    if (nvs_get_u32(nvs_handle, NVS_PROC_MIDPOINT_KEY, &mid_bits) == ESP_OK) {
        memcpy(&settings->midpoint, &mid_bits, sizeof(float));
    }

    len = sizeof(settings->color_method);
    nvs_get_str(nvs_handle, NVS_PROC_COLOR_METHOD_KEY, settings->color_method, &len);

    uint8_t compress_dr = 0;
    if (nvs_get_u8(nvs_handle, NVS_PROC_COMPRESS_DR_KEY, &compress_dr) == ESP_OK) {
        settings->compress_dynamic_range = (compress_dr != 0);
    }

    len = sizeof(settings->dither_algorithm);
    nvs_get_str(nvs_handle, NVS_PROC_DITHER_ALGO_KEY, settings->dither_algorithm, &len);

    len = sizeof(settings->scale_mode);
    nvs_get_str(nvs_handle, NVS_PROC_SCALE_MODE_KEY, settings->scale_mode, &len);

    len = sizeof(settings->background_color);
    nvs_get_str(nvs_handle, NVS_PROC_BG_COLOR_KEY, settings->background_color, &len);

    nvs_close(nvs_handle);

    return ESP_OK;
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t processing_settings_from_json_checked(cJSON *json, processing_settings_t *settings)
{
    if (!settings || !cJSON_IsObject(json)) return ESP_ERR_INVALID_ARG;
    processing_settings_t candidate = *settings;
    struct { const char *key; float *value; } numbers[] = {
        {"exposure", &candidate.exposure}, {"saturation", &candidate.saturation},
        {"contrast", &candidate.contrast}, {"strength", &candidate.strength},
        {"shadowBoost", &candidate.shadow_boost}, {"highlightCompress", &candidate.highlight_compress},
        {"midpoint", &candidate.midpoint}};
    for (size_t i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
        cJSON *item = cJSON_GetObjectItem(json, numbers[i].key);
        if (!item) continue;
        if (!cJSON_IsNumber(item) || !isfinite(item->valuedouble) || fabs(item->valuedouble) > FLT_MAX)
            return ESP_ERR_INVALID_ARG;
        *numbers[i].value = (float)item->valuedouble;
    }
    static const char *tone[] = {"contrast", "scurve", NULL};
    static const char *color[] = {"rgb", "lab", NULL};
    static const char *dither[] = {"floyd-steinberg", "stucki", "burkes", "sierra", NULL};
    static const char *scale[] = {"cover", "fit", NULL};
    static const char *background[] = {"white", "black", NULL};
    struct { const char *key; char *value; size_t capacity; const char *const *allowed; } strings[] = {
        {"toneMode", candidate.tone_mode, sizeof(candidate.tone_mode), tone},
        {"colorMethod", candidate.color_method, sizeof(candidate.color_method), color},
        {"ditherAlgorithm", candidate.dither_algorithm, sizeof(candidate.dither_algorithm), dither},
        {"scaleMode", candidate.scale_mode, sizeof(candidate.scale_mode), scale},
        {"backgroundColor", candidate.background_color, sizeof(candidate.background_color), background}};
    for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); ++i) {
        cJSON *item = cJSON_GetObjectItem(json, strings[i].key);
        if (!item) continue;
        if (!cJSON_IsString(item) || !item->valuestring) return ESP_ERR_INVALID_ARG;
        size_t length = strnlen(item->valuestring, strings[i].capacity);
        if (length == strings[i].capacity) return ESP_ERR_INVALID_SIZE;
        bool supported = false;
        for (size_t j = 0; strings[i].allowed[j]; ++j)
            if (!strcmp(item->valuestring, strings[i].allowed[j])) supported = true;
        if (!supported) return ESP_ERR_INVALID_ARG;
        memcpy(strings[i].value, item->valuestring, length + 1);
    }
    cJSON *boolean = cJSON_GetObjectItem(json, "compressDynamicRange");
    if (boolean) {
        if (!cJSON_IsBool(boolean)) return ESP_ERR_INVALID_ARG;
        candidate.compress_dynamic_range = cJSON_IsTrue(boolean);
    }
    *settings = candidate;
    return ESP_OK;
}
#endif

void processing_settings_from_json(cJSON *json, processing_settings_t *settings)
{
    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "exposure")) && cJSON_IsNumber(item))
        settings->exposure = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "saturation")) && cJSON_IsNumber(item))
        settings->saturation = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "toneMode")) && cJSON_IsString(item))
        strncpy(settings->tone_mode, item->valuestring, sizeof(settings->tone_mode) - 1);
    if ((item = cJSON_GetObjectItem(json, "contrast")) && cJSON_IsNumber(item))
        settings->contrast = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "strength")) && cJSON_IsNumber(item))
        settings->strength = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "shadowBoost")) && cJSON_IsNumber(item))
        settings->shadow_boost = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "highlightCompress")) && cJSON_IsNumber(item))
        settings->highlight_compress = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "midpoint")) && cJSON_IsNumber(item))
        settings->midpoint = (float) item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "colorMethod")) && cJSON_IsString(item))
        strncpy(settings->color_method, item->valuestring, sizeof(settings->color_method) - 1);
    if ((item = cJSON_GetObjectItem(json, "compressDynamicRange")) && cJSON_IsBool(item))
        settings->compress_dynamic_range = cJSON_IsTrue(item);
    if ((item = cJSON_GetObjectItem(json, "ditherAlgorithm")) && cJSON_IsString(item))
        strncpy(settings->dither_algorithm, item->valuestring,
                sizeof(settings->dither_algorithm) - 1);
    if ((item = cJSON_GetObjectItem(json, "scaleMode")) && cJSON_IsString(item) &&
        (strcmp(item->valuestring, "cover") == 0 || strcmp(item->valuestring, "fit") == 0))
        strncpy(settings->scale_mode, item->valuestring, sizeof(settings->scale_mode) - 1);
    if ((item = cJSON_GetObjectItem(json, "backgroundColor")) && cJSON_IsString(item)) {
        // Allowlist: guarantees termination and rejects unsupported names
        // (the letterbox background is white or black only)
        static const char *bg_names[] = {"black", "white"};
        for (size_t i = 0; i < sizeof(bg_names) / sizeof(bg_names[0]); i++) {
            if (strcmp(item->valuestring, bg_names[i]) == 0) {
                snprintf(settings->background_color, sizeof(settings->background_color), "%s",
                         bg_names[i]);
                break;
            }
        }
    }
}

char *processing_settings_to_json(const processing_settings_t *settings)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!settings || !isfinite(settings->exposure) || !isfinite(settings->saturation) ||
        !isfinite(settings->contrast) || !isfinite(settings->strength) || !isfinite(settings->shadow_boost) ||
        !isfinite(settings->highlight_compress) || !isfinite(settings->midpoint) ||
        !memchr(settings->tone_mode, 0, sizeof(settings->tone_mode)) ||
        !memchr(settings->color_method, 0, sizeof(settings->color_method)) ||
        !memchr(settings->dither_algorithm, 0, sizeof(settings->dither_algorithm)) ||
        !memchr(settings->scale_mode, 0, sizeof(settings->scale_mode)) ||
        !memchr(settings->background_color, 0, sizeof(settings->background_color))) return NULL;
#endif
    cJSON *json = cJSON_CreateObject();
    if (!json) {
        return NULL;
    }

#if CONFIG_BOARD_DRIVER_READ_PICO
#define JSON_ADD(call) do { if (!(call)) { cJSON_Delete(json); return NULL; } } while (0)
#else
#define JSON_ADD(call) (void)(call)
#endif
    JSON_ADD(cJSON_AddNumberToObject(json, "exposure", settings->exposure));
    JSON_ADD(cJSON_AddNumberToObject(json, "saturation", settings->saturation));
    JSON_ADD(cJSON_AddStringToObject(json, "toneMode", settings->tone_mode));
    JSON_ADD(cJSON_AddNumberToObject(json, "contrast", settings->contrast));
    JSON_ADD(cJSON_AddNumberToObject(json, "strength", settings->strength));
    JSON_ADD(cJSON_AddNumberToObject(json, "shadowBoost", settings->shadow_boost));
    JSON_ADD(cJSON_AddNumberToObject(json, "highlightCompress", settings->highlight_compress));
    JSON_ADD(cJSON_AddNumberToObject(json, "midpoint", settings->midpoint));
    JSON_ADD(cJSON_AddStringToObject(json, "colorMethod", settings->color_method));
    JSON_ADD(cJSON_AddStringToObject(json, "ditherAlgorithm", settings->dither_algorithm));
    JSON_ADD(cJSON_AddBoolToObject(json, "compressDynamicRange", settings->compress_dynamic_range));
    JSON_ADD(cJSON_AddStringToObject(json, "scaleMode", settings->scale_mode));
    JSON_ADD(cJSON_AddStringToObject(json, "backgroundColor", settings->background_color));
#undef JSON_ADD

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    return json_str;
}
