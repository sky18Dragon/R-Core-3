#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password);
// Caller capacities: WIFI_SSID_MAX_LEN/WIFI_PASS_MAX_LEN. Read Pico leaves
// both outputs unchanged on errors; only an absent/empty pair is NOT_FOUND.
esp_err_t wifi_manager_load_credentials(char *ssid, char *password);
#ifdef __cplusplus
}
#endif
