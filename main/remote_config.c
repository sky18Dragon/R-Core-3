#include "remote_config.h"
#include "config_document.h"
#include <string.h>

esp_err_t remote_config_capture(char *target, size_t capacity, bool *seen, const char *value)
{
    if (!target || !seen || !value || !capacity) return ESP_ERR_INVALID_ARG;
    if (*seen) return ESP_ERR_INVALID_RESPONSE;
    *seen = true;
    size_t length = strnlen(value, capacity);
    if (length == capacity) return ESP_ERR_INVALID_SIZE;
    memcpy(target, value, length + 1);
    return ESP_OK;
}

esp_err_t remote_config_apply(const char *payload, const remote_config_ops_t *ops,
                              remote_config_result_t *result)
{
    if (!payload || !ops || !ops->apply || !ops->finish || !result) return ESP_ERR_INVALID_ARG;
    *result = (remote_config_result_t){.failed_stage = REMOTE_CONFIG_PARSE};
    size_t length = strnlen(payload, REMOTE_CONFIG_CAPACITY);
    if (length == REMOTE_CONFIG_CAPACITY) return ESP_ERR_INVALID_SIZE;
    cJSON *root = NULL;
    esp_err_t parsed = config_document_parse(payload, length, &root);
    // Preserve the remote empty/invalid-document contract and its 2048 limit.
    if (parsed != ESP_OK) return ESP_ERR_INVALID_RESPONSE;
    esp_err_t error = ESP_ERR_INVALID_RESPONSE;
    static const char *names[] = {"config", "processing_settings", "color_palette"};
    cJSON *groups[3];
    for (unsigned i = 0; i < 3; ++i) {
        groups[i] = cJSON_GetObjectItem(root, names[i]);
        if (groups[i] && !cJSON_IsObject(groups[i])) goto done;
    }
    error = ESP_OK;
    for (unsigned i = 0; i < 3; ++i) {
        if (!groups[i] || !groups[i]->child) continue;
        result->failed_stage = (remote_config_stage_t)i;
        error = ops->apply(ops->context, (remote_config_stage_t)i, groups[i]);
        if (error != ESP_OK) goto done;
        result->completed |= UINT32_C(1) << i;
    }
    if (result->completed) {
        result->failed_stage = REMOTE_CONFIG_REVISION;
        error = ops->finish(ops->context);
        if (error != ESP_OK) goto done;
        result->revision_confirmed = true;
    }
    result->failed_stage = REMOTE_CONFIG_NONE;
done:
    cJSON_Delete(root);
    return error;
}
