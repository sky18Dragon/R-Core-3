#ifndef WIFI_NETWORK_JSON_H
#define WIFI_NETWORK_JSON_H
#include "cJSON.h"
#include "esp_err.h"
#include "wifi_network.h"
// Parse only network fields, no IO. Outputs remain unchanged on error.
// Effective current-or-request validation belongs to wifi_network_patch.
esp_err_t wifi_network_json_patch(const cJSON *root, uint32_t *fields, wifi_network_t *patch);
#endif
