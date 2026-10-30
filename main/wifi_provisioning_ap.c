#include "wifi_provisioning_ap.h"
#include <stdatomic.h>
#include <string.h>
#include "dns_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include "provisioning_session.h"
#include "wifi_manager.h"
#include "read_pico_http_service.h"

enum { AP_IDLE, AP_STARTING, AP_ACTIVE, AP_STOPPING, AP_FAILED };
static atomic_int s_state = AP_IDLE;
static atomic_flag s_control = ATOMIC_FLAG_INIT;
static bool s_http_claimed, s_http_attempted;
static bool s_radio_owned, s_dns_attempted, s_uncertain;
static uint32_t s_manager_owner;
static const char *TAG = "wifi_prov_ap";

bool wifi_provisioning_ap_active(void) { return s_state == AP_ACTIVE; }

static esp_err_t stop_owned(void)
{
    provisioning_session_cancel();
    s_state = AP_STOPPING;
    if (s_uncertain) { s_state = AP_FAILED; return ESP_ERR_INVALID_STATE; }
    esp_err_t error = ESP_OK;
    if (s_manager_owner) {
        error = wifi_manager_provisioning_quiesce(s_manager_owner);
        if (error != ESP_OK) goto failed; // No SDK teardown while a candidate owns the mutex.
    } else if (s_radio_owned || s_http_attempted || s_dns_attempted) {
        error = ESP_ERR_INVALID_STATE; goto failed; // Never guess control without a manager owner.
    }
    if (s_dns_attempted) {
        error = dns_server_stop_checked();
        if (error != ESP_OK) goto failed;
        s_dns_attempted = false;
    }
    if (s_http_attempted) {
        error = read_pico_http_stop(READ_PICO_HTTP_PROVISION);
        if (error != ESP_OK) goto failed;
        s_http_attempted = false;
    }
    if (s_radio_owned) {
        error = esp_wifi_stop();
        if (error != ESP_OK && error != ESP_ERR_WIFI_NOT_STARTED) goto failed;
        s_radio_owned = false;
    }
    if (s_http_claimed) {
        error = read_pico_http_release(READ_PICO_HTTP_PROVISION);
        if (error != ESP_OK) goto failed;
        s_http_claimed = false;
    }
    s_state = AP_IDLE;
    return ESP_OK;
failed:
    s_state = AP_FAILED;
    return error; // Retain exactly the objects whose exit was not confirmed.
}

esp_err_t wifi_provisioning_ap_start(const char *ssid,
                                    esp_err_t (*register_routes)(httpd_handle_t))
{
    if (!ssid || !register_routes) return ESP_ERR_INVALID_ARG;
    // Copy before any SDK/RTOS call; the caller's buffer is not our lifetime.
    size_t length = strnlen(ssid, 33);
    if (!length || length > 32) return ESP_ERR_INVALID_SIZE;
    char name[33]; memcpy(name, ssid, length); name[length] = 0;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t error = ESP_ERR_INVALID_STATE;
    bool manager_acquired = false;
    if (s_state != AP_IDLE) goto finished;
    error = read_pico_http_claim(READ_PICO_HTTP_PROVISION);
    if (error != ESP_OK) goto finished; // No radio change while the frame owns HTTP.
    s_http_claimed = true;
    esp_netif_t *netif = NULL;
    error = wifi_manager_provisioning_begin(&s_manager_owner, &netif);
    if (error != ESP_OK) {
        esp_err_t released = read_pico_http_release(READ_PICO_HTTP_PROVISION);
        if (released == ESP_OK) s_http_claimed = false;
        else s_state = AP_FAILED;
        goto finished;
    }
    manager_acquired = true; // Finish in this task on every returned path.
    s_state = AP_STARTING;
    provisioning_session_cancel();
    error = esp_wifi_stop();
    if (error != ESP_OK && error != ESP_ERR_WIFI_NOT_STARTED) {
        // No AP resource was acquired. Do not guess that another stop repairs it.
        s_uncertain = true; s_state = AP_FAILED; goto finished;
    }
    s_radio_owned = true;
#define AP_REQUIRE(call) do { error = (call); if (error != ESP_OK) goto failed; } while (0)
    AP_REQUIRE(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t config = {.ap = {.channel = 1, .password = "", .max_connection = 4,
                                  .authmode = WIFI_AUTH_OPEN}};
    memcpy(config.ap.ssid, name, length); config.ap.ssid_len = length;
    AP_REQUIRE(esp_wifi_set_config(WIFI_IF_AP, &config));
    AP_REQUIRE(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
    error = esp_netif_dhcps_stop(netif);
    if (error != ESP_OK && error != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) goto failed;
    esp_netif_ip_info_t ip = {0};
    IP4_ADDR(&ip.ip, 192, 168, 4, 1); IP4_ADDR(&ip.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip.netmask, 255, 255, 255, 0);
    AP_REQUIRE(esp_netif_set_ip_info(netif, &ip));
    AP_REQUIRE(esp_netif_dhcps_start(netif));
    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    http_config.server_port = 80; http_config.max_uri_handlers = 16;
    s_http_attempted = true;
    AP_REQUIRE(read_pico_http_start(READ_PICO_HTTP_PROVISION, &http_config, register_routes));
    s_dns_attempted = true; // Even failed DNS start can have an unconfirmed close.
    AP_REQUIRE(dns_server_start());
    if (!provisioning_session_begin()) { error = ESP_ERR_INVALID_STATE; goto failed; }
    AP_REQUIRE(wifi_manager_provisioning_finish(s_manager_owner, WIFI_PROVISION_READY));
    manager_acquired = false;
    s_state = AP_ACTIVE;
    ESP_LOGI(TAG, "Provisioning AP/HTTP/DNS ready");
    error = ESP_OK;
    goto finished;
failed:
    {
        esp_err_t cleanup = stop_owned();
        ESP_LOGE(TAG, "AP startup failed (%s), cleanup=%s; no erase or restart",
                 esp_err_to_name(error), esp_err_to_name(cleanup));
    }
finished:
    if (manager_acquired) {
        esp_err_t released = wifi_manager_provisioning_finish(s_manager_owner,
            s_state == AP_IDLE ? WIFI_PROVISION_STOPPED : WIFI_PROVISION_FAILED);
        if (released == ESP_OK && s_state == AP_IDLE) s_manager_owner = 0;
        if (released != ESP_OK) {
            s_state = AP_FAILED;
            if (error == ESP_OK) error = released;
            ESP_LOGE(TAG, "AP manager reservation unconfirmed: %s", esp_err_to_name(released));
        }
    }
    atomic_flag_clear(&s_control);
    return error;
#undef AP_REQUIRE
}

esp_err_t wifi_provisioning_ap_stop(void)
{
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    // Idle stop must not power down some later unrelated station connection.
    esp_err_t error = s_state == AP_IDLE ? ESP_OK : stop_owned();
    if (s_manager_owner) {
        esp_err_t released = wifi_manager_provisioning_finish(s_manager_owner,
            s_state == AP_IDLE ? WIFI_PROVISION_STOPPED : WIFI_PROVISION_FAILED);
        if (released == ESP_OK && s_state == AP_IDLE) s_manager_owner = 0;
        if (released != ESP_OK) {
            s_state = AP_FAILED;
            if (error == ESP_OK) error = released;
        }
    }
    atomic_flag_clear(&s_control);
    return error;
}
