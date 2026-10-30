#include "config_document.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

esp_err_t config_document_receive(char *buffer, size_t capacity, size_t length,
                                  const config_document_source_t *source)
{
    if (!buffer || !source || !source->receive) return ESP_ERR_INVALID_ARG;
    if (!length || length >= capacity || length >= CONFIG_DOCUMENT_CAPACITY) return ESP_ERR_INVALID_SIZE;
    size_t received = 0;
    unsigned timeouts = 0;
    while (received < length) {
        size_t got = 0;
        esp_err_t error = source->receive(source->context, buffer + received, length - received, &got);
        if (error == ESP_ERR_TIMEOUT) {
            if (++timeouts < 8) continue;
            return error;
        }
        if (error != ESP_OK) return error;
        if (!got || got > length - received) return ESP_ERR_INVALID_RESPONSE;
        received += got;
    }
    buffer[length] = '\0';
    return ESP_OK;
}

// Preflight bounds the fixed cJSON parser's recursion and prevents decoded NUL
// hiding suffixes from its C-string interface. Actual grammar is checked below.
static bool safe_json_text(const char *text, size_t length)
{
    unsigned depth = 0;
    bool quoted = false;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (quoted) {
            if (c == '\\') {
                if (++i >= length) return false;
                if (text[i] == 'u' && i + 4 < length && !memcmp(text + i + 1, "0000", 4)) return false;
            } else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') {
            if (++depth > 16) return false;
        } else if (c == '}' || c == ']') {
            if (!depth) return false;
            --depth;
        }
    }
    return !quoted && !depth;
}
static bool unambiguous_tree(const cJSON *node)
{
    if (cJSON_IsNumber(node) && !isfinite(node->valuedouble)) return false;
    for (const cJSON *child = node->child; child; child = child->next) {
        if (cJSON_IsObject(node)) {
            if (!child->string) return false;
            for (const cJSON *previous = node->child; previous != child; previous = previous->next)
                if (!strcasecmp(previous->string, child->string)) return false;
        }
        if (!unambiguous_tree(child)) return false;
    }
    return true;
}
esp_err_t config_document_parse(const char *text, size_t length, cJSON **out)
{
    if (!text || !out) return ESP_ERR_INVALID_ARG;
    if (!length || length >= CONFIG_DOCUMENT_CAPACITY) return ESP_ERR_INVALID_SIZE;
    if (text[length] != '\0' || memchr(text, 0, length) || !safe_json_text(text, length)) return ESP_ERR_INVALID_RESPONSE;
    cJSON *root = cJSON_ParseWithLengthOpts(text, length + 1, NULL, true);
    if (!root) return ESP_ERR_INVALID_RESPONSE; // cJSON has no separate allocation error result.
    if (!cJSON_IsObject(root) || !unambiguous_tree(root)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_RESPONSE;
    }
    *out = root;
    return ESP_OK;
}
