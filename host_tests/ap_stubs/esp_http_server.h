#pragma once
#include "esp_err.h"
typedef void *httpd_handle_t;
typedef struct { unsigned server_port, max_uri_handlers; } httpd_config_t;
#define HTTPD_DEFAULT_CONFIG() ((httpd_config_t){80, 8})
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t httpd_start(httpd_handle_t *, const httpd_config_t *);
esp_err_t httpd_stop(httpd_handle_t);
#ifdef __cplusplus
}
#endif
