#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t http_server_init(void);
esp_err_t http_server_stop(void);
void http_server_set_ready(void);
esp_err_t http_server_set_ready_checked(void);
bool http_server_is_ready(void);

#endif
