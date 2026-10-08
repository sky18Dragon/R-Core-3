#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cJSON.h"
#include "esp_err.h"

#define REMOTE_CONFIG_CAPACITY 2048
typedef enum { REMOTE_CONFIG_GENERAL, REMOTE_CONFIG_PROCESSING, REMOTE_CONFIG_PALETTE,
               REMOTE_CONFIG_REVISION, REMOTE_CONFIG_PARSE, REMOTE_CONFIG_NONE } remote_config_stage_t;
typedef struct {
    void *context;
    esp_err_t (*apply)(void *, remote_config_stage_t, cJSON *);
    esp_err_t (*finish)(void *);
} remote_config_ops_t;
typedef struct {
    uint32_t completed;
    remote_config_stage_t failed_stage;
    bool revision_confirmed;
} remote_config_result_t;

// Header capture refuses truncation/duplicates. Only successful copies change
// target; seen records even an empty first header. Reset per response, not retry.
esp_err_t remote_config_capture(char *target, size_t capacity, bool *seen, const char *value);
// No ownership of input/ops. All structural checks precede callbacks; partial
// successful effects are retained, not rolled back. Unknown root metadata is
// ignored like upstream; empty/unknown-only payloads do not call finish.
esp_err_t remote_config_apply(const char *payload, const remote_config_ops_t *ops,
                              remote_config_result_t *result);
