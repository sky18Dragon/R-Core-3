#include "mdns_service.h"

#include <stdio.h>
#include <string.h>

#include "board_hal.h"
#include "config_manager.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "mdns.h"
#include "utils.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include <stdatomic.h>
static atomic_flag mdns_control = ATOMIC_FLAG_INIT;
static esp_err_t register_name(const char *name, bool replace);
#endif

static const char *TAG = "mdns_service";

static esp_err_t mdns_register_services(const char *device_name)
{
    char hostname[64];
    sanitize_hostname(device_name, hostname, sizeof(hostname));

    ESP_LOGI(TAG, "Device name: %s, hostname: %s", device_name, hostname);

    esp_err_t err = mdns_hostname_set(hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mDNS hostname: %s", esp_err_to_name(err));
        return err;
    }

    err = mdns_instance_name_set(device_name);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mDNS instance name: %s", esp_err_to_name(err));
        return err;
    }

    // Add HTTP service
    err = mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add HTTP service: %s", esp_err_to_name(err));
        return err;
    }

    // Add photoframe-specific service for app auto-discovery
    const esp_app_desc_t *app_desc = esp_app_get_description();
    char mdns_host[70];
    snprintf(mdns_host, sizeof(mdns_host), "%s.local", hostname);
    mdns_txt_item_t txt[] = {
        {"name", device_name},
        {"host", mdns_host},
        {"board", BOARD_HAL_NAME},
        {"version", app_desc->version},
    };
    err = mdns_service_add(NULL, "_esp32-pframe", "_tcp", 80, txt, 4);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add photoframe service: %s", esp_err_to_name(err));
        // Non-fatal: HTTP service still works
    }

    ESP_LOGI(TAG, "mDNS services registered, accessible at: http://%s.local", hostname);
    return ESP_OK;
}

esp_err_t mdns_service_init(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    char name[DEVICE_NAME_MAX_LEN];
    esp_err_t copied = config_manager_copy_device_name(name, sizeof(name));
    if (copied != ESP_OK) return copied;
    return register_name(name, false);
#else
    const char *name = config_manager_get_device_name();
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS initialization failed: %s", esp_err_to_name(err));
        return err;
    }

    return mdns_register_services(name);
#endif
}

esp_err_t mdns_service_update_hostname(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    char name[DEVICE_NAME_MAX_LEN];
    esp_err_t copied = config_manager_copy_device_name(name, sizeof(name));
    if (copied != ESP_OK) return copied; // Do not free the current service on a busy copy.
    return mdns_service_update_hostname_named(name);
#else
    const char *name = config_manager_get_device_name();
    // Free existing mDNS service to send goodbye packets, then reinitialize
    mdns_free();

    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reinitialize mDNS: %s", esp_err_to_name(err));
        return err;
    }

    return mdns_register_services(name);
#endif
}
#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t register_name(const char *name, bool replace)
{
    if (!name || !*name) return ESP_ERR_INVALID_ARG;
    size_t length = strnlen(name, DEVICE_NAME_MAX_LEN);
    if (length == DEVICE_NAME_MAX_LEN) return ESP_ERR_INVALID_SIZE;
    char snapshot[DEVICE_NAME_MAX_LEN] = {0};
    memcpy(snapshot, name, length + 1);
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)snapshot[i] < 0x20 || (unsigned char)snapshot[i] == 0x7f) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&mdns_control)) return ESP_ERR_INVALID_STATE;
    if (replace) mdns_free();
    esp_err_t err = mdns_init();
    if (err == ESP_OK) err = mdns_register_services(snapshot);
    atomic_flag_clear(&mdns_control);
    return err;
}
esp_err_t mdns_service_update_hostname_named(const char *name)
{
    return register_name(name, true);
}
#endif
