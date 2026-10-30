#pragma once

#include <stdbool.h>
#include "esp_http_server.h"

// Scoped ownership of the existing two HTTP services, not a product state store.
typedef enum { READ_PICO_HTTP_NONE, READ_PICO_HTTP_FRAME, READ_PICO_HTTP_PROVISION }
    read_pico_http_role_t;
esp_err_t read_pico_http_claim(read_pico_http_role_t role);
esp_err_t read_pico_http_start(read_pico_http_role_t role, const httpd_config_t *config,
                              esp_err_t (*routes)(httpd_handle_t));
// Stop retains the role until release, including when there was no HTTP handle.
// SDK stop failures retain the exact handle. No forced deletion or time guarantee.
esp_err_t read_pico_http_stop(read_pico_http_role_t role);
esp_err_t read_pico_http_release(read_pico_http_role_t role);
esp_err_t read_pico_http_mark_ready(read_pico_http_role_t role);
bool read_pico_http_active(read_pico_http_role_t role);
bool read_pico_http_ready(read_pico_http_role_t role);
bool read_pico_http_accepts(read_pico_http_role_t role, httpd_handle_t handle);
