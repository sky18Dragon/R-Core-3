#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t wifi_provisioning_init(void);
esp_err_t wifi_provisioning_start_ap(void);
esp_err_t wifi_provisioning_stop_ap(void);
bool wifi_provisioning_is_active(void);
bool wifi_provisioning_is_provisioned(void);
bool wifi_provisioning_session_complete(void);

// Read Pico only: setup routes served by the frame HTTP server while the device
// hotspot is on (Wi-Fi scan and the test-then-save of home Wi-Fi, same checks).
struct httpd_req;
esp_err_t wifi_provisioning_scan_request(struct httpd_req *req);
esp_err_t wifi_provisioning_save_request(struct httpd_req *req);

#endif
