#include "request_config.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "color_palette.h"
#include "config_apply.h"
#include "config_manager.h"
#include "processing_settings.h"

void request_config_release(request_config_t *snapshot)
{
    if (!snapshot) return;
    cJSON_free(snapshot->processing);
    cJSON_free(snapshot->palette);
    *snapshot = (request_config_t){0};
}

static esp_err_t capture(void *context)
{
    int64_t revision;
    esp_err_t err = config_manager_copy_config_last_updated(&revision);
    if (err != ESP_OK) return err;
    processing_settings_t processing;
    err = processing_settings_load(&processing);
    if (err != ESP_OK) return err;
    color_palette_t palette;
    err = color_palette_load(&palette);
    if (err != ESP_OK) return err;
    request_config_t candidate = {0};
    int n = snprintf(candidate.revision, sizeof(candidate.revision), "%lld", (long long)revision);
    if (n < 0 || (size_t)n >= sizeof(candidate.revision)) return ESP_ERR_INVALID_SIZE;
    candidate.processing = processing_settings_to_json(&processing);
    if (!candidate.processing) return ESP_ERR_NO_MEM;
    candidate.palette = color_palette_to_json(&palette);
    if (!candidate.palette) {
        request_config_release(&candidate);
        return ESP_ERR_NO_MEM;
    }
    *(request_config_t *)context = candidate;
    return ESP_OK;
}

esp_err_t request_config_capture(request_config_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    return config_apply_run(capture, out);
}

esp_err_t request_header_put_checked(const request_header_ops_t *ops,
                                     const char *key, const char *value)
{
    if (!ops || !ops->remove || !ops->get || !ops->set || !key || !*key || !value)
        return ESP_ERR_INVALID_ARG;
    for (const unsigned char *p = (const unsigned char *)key; *p; ++p) {
        bool alnum = (*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'Z') ||
                     (*p >= 'a' && *p <= 'z');
        if (!alnum && !strchr("!#$%&'*+-.^_`|~", *p)) return ESP_ERR_INVALID_ARG;
    }
    for (const unsigned char *p = (const unsigned char *)value; *p; ++p)
        if ((*p < 32 && *p != '\t') || *p == 127) return ESP_ERR_INVALID_ARG;
    esp_err_t err = ops->remove(ops->context, key);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) return err;
    const char *readback = NULL;
    err = ops->get(ops->context, key, &readback);
    if (err != ESP_ERR_NOT_FOUND || readback) {
        if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) return err;
        return ESP_ERR_INVALID_RESPONSE; // Avoid SDK's unchecked existing-value update.
    }
    err = ops->set(ops->context, key, value);
    if (err != ESP_OK) return err;
    readback = NULL;
    err = ops->get(ops->context, key, &readback);
    if (err != ESP_OK) return err;
    if (!readback || strcmp(readback, value) != 0) return ESP_ERR_INVALID_RESPONSE;
    return ESP_OK;
}

esp_err_t request_config_set_headers(const request_config_t *snapshot,
                                     const request_header_ops_t *ops)
{
    if (!snapshot || !snapshot->processing || !snapshot->palette || !snapshot->revision[0] ||
        !memchr(snapshot->revision, 0, sizeof(snapshot->revision)) ||
        !ops || !ops->remove || !ops->get || !ops->set) return ESP_ERR_INVALID_ARG;
    const char *keys[] = {"X-Config-Last-Updated", "X-Processing-Settings", "X-Color-Palette"};
    const char *values[] = {snapshot->revision, snapshot->processing, snapshot->palette};
    for (size_t i = 0; i < 3; ++i) {
        esp_err_t err = request_header_put_checked(ops, keys[i], values[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
