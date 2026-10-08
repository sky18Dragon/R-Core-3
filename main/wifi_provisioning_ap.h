#pragma once
#include <stdbool.h>
#include "esp_err.h"
#include "esp_http_server.h"
#ifdef __cplusplus
extern "C" {
#endif
// Read Pico SDK adapter; owns only this provisioning server/DNS and radio use.
// Called by the original PhotoFrame provisioning entry, never a second app.
esp_err_t wifi_provisioning_ap_start(const char *ssid,
                                    esp_err_t (*register_routes)(httpd_handle_t));
esp_err_t wifi_provisioning_ap_stop(void);
bool wifi_provisioning_ap_active(void);
#ifdef __cplusplus
}
#endif
