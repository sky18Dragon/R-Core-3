#include "wifi_credentials.h"
#include <string.h>
#include "config.h"
#include "nvs.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include "config_read.h"
#include "wifi_text.h"
#endif

esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!wifi_text_credentials_valid(ssid, password)) return ESP_ERR_INVALID_ARG;
#endif
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_str(handle, NVS_WIFI_SSID_KEY, ssid);
    if (err == ESP_OK) err = nvs_set_str(handle, NVS_WIFI_PASS_KEY, password);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err; // Existing two-key contract: errors do not promise rollback.
}

esp_err_t wifi_manager_load_credentials(char *ssid, char *password)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!ssid || !password) return ESP_ERR_INVALID_ARG;
#endif
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;
#if CONFIG_BOARD_DRIVER_READ_PICO
    char loaded_ssid[WIFI_SSID_MAX_LEN] = {0}, loaded_password[WIFI_PASS_MAX_LEN] = {0};
    size_t s = sizeof(loaded_ssid), p = sizeof(loaded_password);
    config_read_guard_t guard = {0, ESP_ERR_NVS_NOT_FOUND, ESP_ERR_INVALID_RESPONSE};
    esp_err_t sr = nvs_get_str(handle, NVS_WIFI_SSID_KEY, loaded_ssid, &s);
    config_read_string(&guard, sr, loaded_ssid, sizeof(loaded_ssid), s);
    esp_err_t pr = nvs_get_str(handle, NVS_WIFI_PASS_KEY, loaded_password, &p);
    config_read_string(&guard, pr, loaded_password, sizeof(loaded_password), p);
    nvs_close(handle);
    if (guard.error) return guard.error;
    if (sr == ESP_ERR_NVS_NOT_FOUND && pr == ESP_ERR_NVS_NOT_FOUND) return ESP_ERR_NVS_NOT_FOUND;
    if (sr != ESP_OK || pr != ESP_OK) return ESP_ERR_INVALID_RESPONSE;
    if (!loaded_ssid[0] && !loaded_password[0]) return ESP_ERR_NVS_NOT_FOUND;
    if (!wifi_text_credentials_valid(loaded_ssid, loaded_password)) return ESP_ERR_INVALID_RESPONSE;
    memcpy(ssid, loaded_ssid, s); memcpy(password, loaded_password, p);
    return ESP_OK;
#else
    size_t s = WIFI_SSID_MAX_LEN;
    err = nvs_get_str(handle, NVS_WIFI_SSID_KEY, ssid, &s);
    if (err == ESP_OK) {
        size_t p = WIFI_PASS_MAX_LEN;
        err = nvs_get_str(handle, NVS_WIFI_PASS_KEY, password, &p);
    }
    nvs_close(handle);
    return err;
#endif
}
