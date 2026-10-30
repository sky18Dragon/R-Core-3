#pragma once
#include <stddef.h>
#include "cJSON.h"
#include "esp_err.h"

#define CONFIG_DOCUMENT_CAPACITY 4096
typedef struct {
    void *context;
    esp_err_t (*receive)(void *, char *, size_t, size_t *);
} config_document_source_t;
// Private caller buffer may contain partial input on failure; never parse it.
// Eight total timeout returns end the receive, without resetting on progress.
// SDK calls themselves are not interrupted or given a new hard time bound.
esp_err_t config_document_receive(char *buffer, size_t capacity, size_t length,
                                  const config_document_source_t *source);
// text has length bytes plus a NUL. One object document, no decoded/raw NUL,
// depth >16, duplicate case-insensitive keys or nonfinite values. Caller owns
// the tree only on success; output is untouched on failure (including OOM).
esp_err_t config_document_parse(const char *text, size_t length, cJSON **out);
