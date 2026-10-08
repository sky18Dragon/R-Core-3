#include "config_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "board_hal.h"
#include "config.h"
#include "esp_log.h"
#include "nvs.h"
#include "storage.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include <stdatomic.h>
#include "config_read.h"
#include "wifi_text.h"
#endif

static const char *TAG = "config_manager";

// General
static char device_name[DEVICE_NAME_MAX_LEN] = {0};
static char tz_string[TIMEZONE_MAX_LEN] = {0};
static display_orientation_t display_orientation = DISPLAY_ORIENTATION_LANDSCAPE;
static int display_rotation_deg = BOARD_HAL_DISPLAY_ROTATION_DEG;
static char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
static char wifi_password[WIFI_PASS_MAX_LEN] = {0};

// Advanced network settings (collapsed section in the UI): custom NTP server,
// static IP instead of DHCP, and DNS override
static char ntp_server[NTP_SERVER_MAX_LEN] = {0};
#if CONFIG_BOARD_DRIVER_READ_PICO
// Protect checked name/network/time/revision and loading, not all caches/libc.
static atomic_flag time_config_control = ATOMIC_FLAG_INIT;
static bool timezone_applied;
static bool device_hostnames_confirmed;
static esp_err_t store_device_name_locked(const char *candidate)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_str(handle, NVS_DEVICE_NAME_KEY, candidate);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) {
        memcpy(device_name, candidate, sizeof(device_name));
        device_hostnames_confirmed = false;
    }
    return err;
}
static esp_err_t validate_device_name(const char *name, size_t *length)
{
    if (!name || !*name) return ESP_ERR_INVALID_ARG;
    size_t n = strnlen(name, DEVICE_NAME_MAX_LEN);
    if (n == DEVICE_NAME_MAX_LEN) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < n; ++i)
        if ((unsigned char)name[i] < 0x20 || (unsigned char)name[i] == 0x7f) return ESP_ERR_INVALID_ARG;
    *length = n;
    return ESP_OK;
}
static esp_err_t validate_timezone(const char *tz, size_t *length)
{
    if (!tz || !*tz) return ESP_ERR_INVALID_ARG;
    size_t n = strnlen(tz, TIMEZONE_MAX_LEN);
    if (n == TIMEZONE_MAX_LEN) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < n; ++i)
        if ((unsigned char)tz[i] < 0x20 || (unsigned char)tz[i] > 0x7e) return ESP_ERR_INVALID_ARG;
    *length = n;
    return ESP_OK; // Preserve upstream shape-only POSIX grammar policy.
}
static esp_err_t apply_loaded_timezone(void)
{
    timezone_applied = false;
    if (setenv("TZ", tz_string, 1) != 0) return ESP_ERR_NOT_FINISHED;
    tzset(); // libc has no grammar-validation/error result here.
    const char *installed = getenv("TZ");
    if (!installed || strcmp(installed, tz_string)) return ESP_ERR_NOT_FINISHED;
    timezone_applied = true;
    return ESP_OK;
}
static esp_err_t validate_ntp_server(const char *server, size_t *length)
{
    if (!server) return ESP_ERR_INVALID_ARG;
    size_t n = strnlen(server, NTP_SERVER_MAX_LEN);
    if (n == NTP_SERVER_MAX_LEN) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)server[i];
        if (c <= 0x20 || c > 0x7e) return ESP_ERR_INVALID_ARG;
    }
    *length = n;
    return ESP_OK;
}
#endif
static ip_mode_t ip_mode = IP_MODE_DHCP;
static char static_ip[IP_ADDR_STR_MAX_LEN] = {0};
static char static_netmask[IP_ADDR_STR_MAX_LEN] = {0};
static char static_gateway[IP_ADDR_STR_MAX_LEN] = {0};
static char dns_server[IP_ADDR_STR_MAX_LEN] = {0};
#if CONFIG_BOARD_DRIVER_READ_PICO
static void copy_network_locked(wifi_network_t *out)
{
    wifi_network_t candidate = {.static_address = ip_mode == IP_MODE_STATIC};
    memcpy(candidate.ip, static_ip, sizeof(candidate.ip));
    memcpy(candidate.netmask, static_netmask, sizeof(candidate.netmask));
    memcpy(candidate.gateway, static_gateway, sizeof(candidate.gateway));
    memcpy(candidate.dns, dns_server, sizeof(candidate.dns));
    *out = candidate;
}
static void publish_network_locked(const wifi_network_t *candidate)
{
    memcpy(static_ip, candidate->ip, sizeof(static_ip));
    memcpy(static_netmask, candidate->netmask, sizeof(static_netmask));
    memcpy(static_gateway, candidate->gateway, sizeof(static_gateway));
    memcpy(dns_server, candidate->dns, sizeof(dns_server));
    ip_mode = candidate->static_address ? IP_MODE_STATIC : IP_MODE_DHCP;
}
#endif

// Auto Rotate
static bool auto_rotate_enabled = false;
static char cron_rules_store[MAX_CRON_RULES][CRON_RULE_MAX_LEN] = {{0}};
static int cron_rule_count = 0;

static rotation_mode_t rotation_mode =
    ROTATION_MODE_STORAGE;  // Default, will be validated during init

// Auto Rotate - SDCARD
static sd_rotation_mode_t sd_rotation_mode = SD_ROTATION_RANDOM;
static int32_t last_index = -1;

// Auto Rotate - URL
static char image_url[IMAGE_URL_MAX_LEN] = {0};
static uint8_t *ca_cert_der = NULL;  // Heap-allocated DER certificate
static size_t ca_cert_der_len = 0;
static char access_token[ACCESS_TOKEN_MAX_LEN] = {0};
static char http_password[HTTP_PASSWORD_MAX_LEN] = {0};
static char http_header_key[HTTP_HEADER_KEY_MAX_LEN] = {0};
static char http_header_value[HTTP_HEADER_VALUE_MAX_LEN] = {0};
static bool save_downloaded_images = false;
static char image_etag[HTTP_ETAG_MAX_LEN] = {0};

// Home Assistant
static char ha_url[HA_URL_MAX_LEN] = {0};

// AI API Keys
static char openai_api_key[AI_API_KEY_MAX_LEN] = {0};
static char google_api_key[AI_API_KEY_MAX_LEN] = {0};

// Power
static bool deep_sleep_enabled = true;  // Enabled by default

// Debugging
static bool debug_log_enabled = false;

// Config sync
static int64_t config_last_updated = 0;

// ----------------------------------------------------------------------------
// Cron schedule helpers
// ----------------------------------------------------------------------------

// Fill cron_rules_store from a '\n'-separated joined string (skips empty and
// over-long entries, caps at MAX_CRON_RULES).
static void cron_load_from_joined(const char *joined)
{
    cron_rule_count = 0;
    if (!joined) {
        return;
    }
    const char *p = joined;
    while (*p && cron_rule_count < MAX_CRON_RULES) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t) (nl - p) : strlen(p);
        if (len > 0 && len < CRON_RULE_MAX_LEN) {
            memcpy(cron_rules_store[cron_rule_count], p, len);
            cron_rules_store[cron_rule_count][len] = '\0';
            cron_rule_count++;
        }
        if (!nl) {
            break;
        }
        p = nl + 1;
    }
}

// Persist the current cron_rules_store to NVS as a '\n'-joined string.
static void cron_persist(void)
{
    char joined[MAX_CRON_RULES * CRON_RULE_MAX_LEN];
    joined[0] = '\0';
    size_t off = 0;
    for (int i = 0; i < cron_rule_count; i++) {
        int n = snprintf(joined + off, sizeof(joined) - off, "%s%s", i ? "\n" : "",
                         cron_rules_store[i]);
        if (n < 0 || (size_t) n >= sizeof(joined) - off) {
            break;
        }
        off += n;
    }

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        if (cron_rule_count > 0) {
            nvs_set_str(nvs_handle, NVS_ROTATE_CRON_KEY, joined);
        } else {
            nvs_erase_key(nvs_handle, NVS_ROTATE_CRON_KEY);
        }
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

// Convert a legacy rotation interval (seconds) into a single cron expression.
// Mirrors the documented best-effort mapping; falls back to hourly.
static void cron_from_legacy_interval(int seconds, char *out, size_t out_len)
{
    if (seconds >= 3600 && seconds % 3600 == 0) {
        int hours = seconds / 3600;
        if (hours <= 1) {
            snprintf(out, out_len, "0 * *");
        } else if (hours >= 24) {
            snprintf(out, out_len, "0 0 *");
        } else {
            snprintf(out, out_len, "0 */%d *", hours);
        }
    } else if (seconds >= 60 && 3600 % seconds == 0) {
        snprintf(out, out_len, "*/%d * *", seconds / 60);
    } else {
        // Not cleanly expressible as cron (e.g. 90 min) — approximate to hourly.
        snprintf(out, out_len, "0 * *");
    }
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static bool load_cron_valid(const char *joined)
{
    unsigned count = 0;
    for (const char *p = joined; *p;) {
        const char *newline = strchr(p, '\n');
        size_t n = newline ? (size_t)(newline - p) : strlen(p);
        if (n) {
            if (n >= CRON_RULE_MAX_LEN || ++count > MAX_CRON_RULES) return false;
            char rule[CRON_RULE_MAX_LEN];
            memcpy(rule, p, n); rule[n] = 0;
            cron_rule_t parsed;
            if (!cron_parse(rule, &parsed)) return false;
        }
        if (!newline) break;
        p = newline + 1;
    }
    return true;
}
static esp_err_t load_string(config_read_guard_t *g, nvs_handle_t h, const char *key,
                              char *out, size_t *size)
{
    size_t capacity = *size;
    memset(out, 0, capacity);
    esp_err_t err = nvs_get_str(h, key, out, size);
    return config_read_string(g, err, out, capacity, *size);
}
static esp_err_t load_u8(config_read_guard_t *g, nvs_handle_t h, const char *key, uint8_t *out)
{
    esp_err_t err = nvs_get_u8(h, key, out);
    // All currently loaded booleans and two-choice enums have the same range.
    return config_read_integer(g, err, *out, 0, 1);
}
static esp_err_t load_i32(config_read_guard_t *g, nvs_handle_t h, const char *key, int32_t *out)
{
    esp_err_t err = nvs_get_i32(h, key, out);
    if (!err && !strcmp(key, NVS_DISPLAY_ROTATION_DEG_KEY) && *out != 0 && *out != 180)
        return config_read_result(g, ESP_ERR_INVALID_RESPONSE);
    int64_t minimum = !strcmp(key, NVS_LAST_INDEX_KEY) ? -1 : 0;
    if (!strcmp(key, NVS_ROTATE_INTERVAL_KEY)) minimum = 1;
    return config_read_integer(g, err, *out, minimum, INT32_MAX);
}
static esp_err_t load_i64(config_read_guard_t *g, nvs_handle_t h, const char *key, int64_t *out)
{
    esp_err_t err = nvs_get_i64(h, key, out);
    return config_read_integer(g, err, *out, 0, INT64_MAX);
}
static esp_err_t load_blob(config_read_guard_t *g, nvs_handle_t h, const char *key,
                            void *out, size_t *size)
{
    size_t capacity = *size;
    esp_err_t err = nvs_get_blob(h, key, out, size);
    if (out && err == ESP_ERR_NVS_NOT_FOUND) err = ESP_ERR_INVALID_RESPONSE;
    if (!err && (!*size || *size > CA_CERT_MAX_LEN || (out && *size != capacity)))
        err = ESP_ERR_INVALID_SIZE;
    return config_read_result(g, err);
}
static void reset_load_defaults(void)
{
    device_hostnames_confirmed = false;
    // Startup only, before any configuration consumer or network task exists.
#define RESET_TEXT(field, initial) do { memset(field, 0, sizeof(field)); \
    snprintf(field, sizeof(field), "%s", initial); } while (0)
    RESET_TEXT(device_name, DEFAULT_DEVICE_NAME);
    RESET_TEXT(tz_string, DEFAULT_TIMEZONE);
    RESET_TEXT(ntp_server, DEFAULT_NTP_SERVER);
    RESET_TEXT(wifi_ssid, DEFAULT_WIFI_SSID);
    RESET_TEXT(wifi_password, DEFAULT_WIFI_PASSWORD);
    RESET_TEXT(image_url, DEFAULT_IMAGE_URL);
    RESET_TEXT(ha_url, DEFAULT_HA_URL);
    RESET_TEXT(http_password, ""); RESET_TEXT(access_token, "");
    RESET_TEXT(http_header_key, ""); RESET_TEXT(http_header_value, "");
    RESET_TEXT(static_ip, ""); RESET_TEXT(static_netmask, "");
    RESET_TEXT(static_gateway, ""); RESET_TEXT(dns_server, "");
    RESET_TEXT(image_etag, ""); RESET_TEXT(openai_api_key, ""); RESET_TEXT(google_api_key, "");
#undef RESET_TEXT
    free(ca_cert_der); ca_cert_der = NULL; ca_cert_der_len = 0;
    display_orientation = DISPLAY_ORIENTATION_LANDSCAPE;
    display_rotation_deg = BOARD_HAL_DISPLAY_ROTATION_DEG;
    auto_rotate_enabled = save_downloaded_images = debug_log_enabled = false;
    deep_sleep_enabled = true;
    ip_mode = IP_MODE_DHCP;
    // The Read Pico product requires SD and has no volatile/flash fallback.
    // Read config before board/mount IO; card availability is checked later.
    rotation_mode = ROTATION_MODE_STORAGE;
    sd_rotation_mode = SD_ROTATION_RANDOM;
    last_index = -1; config_last_updated = 0;
    memset(cron_rules_store, 0, sizeof(cron_rules_store)); cron_rule_count = 0;
}
// Restrict these aliases to startup loading. Explicit user save APIs below
// retain the upstream contract and are not a migration-approval mechanism.
#define nvs_get_str(...) load_string(&guard, __VA_ARGS__)
#define nvs_get_u8(...) load_u8(&guard, __VA_ARGS__)
#define nvs_get_i32(...) load_i32(&guard, __VA_ARGS__)
#define nvs_get_i64(...) load_i64(&guard, __VA_ARGS__)
#define nvs_get_blob(...) load_blob(&guard, __VA_ARGS__)
#endif

static esp_err_t config_manager_load(void)
{
    ESP_LOGI(TAG, "Initializing config manager");
#if CONFIG_BOARD_DRIVER_READ_PICO
    config_read_guard_t guard = {0, ESP_ERR_NVS_NOT_FOUND, ESP_ERR_INVALID_RESPONSE};
    reset_load_defaults();
#endif

    // Rotation-schedule load is resolved after the read-only NVS handle closes
    // (migration / default seeding may need a read-write handle).
    char cron_buf[MAX_CRON_RULES * CRON_RULE_MAX_LEN] = {0};
    int32_t legacy_interval = 0;
    bool migrate_legacy_interval = false;
    bool seed_default_cron = true;

    nvs_handle_t nvs_handle;
    esp_err_t opened = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (config_read_result(&guard, opened) != ESP_OK && guard.error) return guard.error;
#endif
    if (opened == ESP_OK) {
        // General
        size_t device_name_len = DEVICE_NAME_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_DEVICE_NAME_KEY, device_name, &device_name_len) == ESP_OK) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            size_t validated_length;
            if (validate_device_name(device_name, &validated_length) != ESP_OK)
                config_read_result(&guard, ESP_ERR_INVALID_RESPONSE);
#endif
            ESP_LOGI(TAG, "Loaded device name from NVS: %s", device_name);
        } else {
            strncpy(device_name, DEFAULT_DEVICE_NAME, DEVICE_NAME_MAX_LEN - 1);
            device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No device name in NVS, using default: %s", device_name);
        }

        size_t tz_len = TIMEZONE_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_TIMEZONE_KEY, tz_string, &tz_len) == ESP_OK) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            size_t validated_length;
            if (validate_timezone(tz_string, &validated_length) != ESP_OK)
                config_read_result(&guard, ESP_ERR_INVALID_RESPONSE);
#endif
            ESP_LOGI(TAG, "Loaded timezone from NVS: %s", tz_string);
        } else {
            strncpy(tz_string, DEFAULT_TIMEZONE, TIMEZONE_MAX_LEN - 1);
            tz_string[TIMEZONE_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No timezone in NVS, using default: %s", tz_string);
        }

        size_t http_password_len = HTTP_PASSWORD_MAX_LEN;
        esp_err_t pw_err =
            nvs_get_str(nvs_handle, NVS_HTTP_PASSWORD_KEY, http_password, &http_password_len);
        if (pw_err != ESP_OK) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            // Missing means no password; an actual read error refuses startup
            // after closing this read-only handle, never starts an unguarded API.
            if (pw_err != ESP_ERR_NVS_NOT_FOUND)
                ESP_LOGE(TAG, "HTTP password read failed (%s); configuration startup refused",
                         esp_err_to_name(pw_err));
#else
            // Not found is the normal "auth off" default. Any other error
            // also leaves auth off rather than failing closed: the routes that
            // could repair it, factory reset included, sit behind the same
            // gate, so a lockout here would need a reflash to undo.
            if (pw_err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to read HTTP API password (%s); auth is OFF",
                         esp_err_to_name(pw_err));
            }
#endif
            http_password[0] = '\0';
        }

        size_t ntp_server_len = NTP_SERVER_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_NTP_SERVER_KEY, ntp_server, &ntp_server_len) == ESP_OK) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            size_t validated_length;
            if (validate_ntp_server(ntp_server, &validated_length) != ESP_OK)
                config_read_result(&guard, ESP_ERR_INVALID_RESPONSE);
#endif
            ESP_LOGI(TAG, "Loaded NTP server from NVS: %s", ntp_server);
        } else {
            strncpy(ntp_server, DEFAULT_NTP_SERVER, NTP_SERVER_MAX_LEN - 1);
            ntp_server[NTP_SERVER_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No NTP server in NVS, using default: %s", ntp_server);
        }

        // Advanced network settings
        uint8_t stored_ip_mode = IP_MODE_DHCP;
        if (nvs_get_u8(nvs_handle, NVS_IP_MODE_KEY, &stored_ip_mode) == ESP_OK) {
            ip_mode = (ip_mode_t) stored_ip_mode;
        }
        size_t addr_len = sizeof(static_ip);
        nvs_get_str(nvs_handle, NVS_STATIC_IP_KEY, static_ip, &addr_len);
        addr_len = sizeof(static_netmask);
        nvs_get_str(nvs_handle, NVS_STATIC_NETMASK_KEY, static_netmask, &addr_len);
        addr_len = sizeof(static_gateway);
        nvs_get_str(nvs_handle, NVS_STATIC_GATEWAY_KEY, static_gateway, &addr_len);
        addr_len = sizeof(dns_server);
        nvs_get_str(nvs_handle, NVS_DNS_SERVER_KEY, dns_server, &addr_len);
        if (ip_mode == IP_MODE_STATIC) {
            ESP_LOGI(TAG, "Static IP configured: %s/%s gw %s dns %s", static_ip, static_netmask,
                     static_gateway, dns_server[0] ? dns_server : "(auto)");
        } else if (dns_server[0]) {
            ESP_LOGI(TAG, "DNS override configured: %s", dns_server);
        }

        uint8_t stored_orientation = DISPLAY_ORIENTATION_LANDSCAPE;
        if (nvs_get_u8(nvs_handle, NVS_DISPLAY_ORIENTATION_KEY, &stored_orientation) == ESP_OK) {
            display_orientation = (display_orientation_t) stored_orientation;
            ESP_LOGI(
                TAG, "Loaded display orientation from NVS: %s",
                display_orientation == DISPLAY_ORIENTATION_LANDSCAPE ? "landscape" : "portrait");
        }

        int32_t stored_display_rotation_deg = 0;
        if (nvs_get_i32(nvs_handle, NVS_DISPLAY_ROTATION_DEG_KEY, &stored_display_rotation_deg) ==
            ESP_OK) {
            // Only 0/180 are supported; a legacy 90/270 value would swap the
            // paint geometry away from the native layout the streaming
            // pipeline assumes (see apply_config_from_json). Keep the board
            // default instead.
            if (stored_display_rotation_deg == 0 || stored_display_rotation_deg == 180) {
                display_rotation_deg = stored_display_rotation_deg;
                ESP_LOGI(TAG, "Loaded display rotation from NVS: %d degrees", display_rotation_deg);
            } else {
                ESP_LOGW(TAG, "Ignoring unsupported stored display rotation %ld degrees",
                         (long) stored_display_rotation_deg);
            }
        }

        size_t wifi_ssid_len = WIFI_SSID_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_WIFI_SSID_KEY, wifi_ssid, &wifi_ssid_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded WiFi SSID from NVS: %s", wifi_ssid);
        } else {
            strncpy(wifi_ssid, DEFAULT_WIFI_SSID, WIFI_SSID_MAX_LEN - 1);
            wifi_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No WiFi SSID in NVS, using default: %s", wifi_ssid);
        }

        size_t wifi_pass_len = WIFI_PASS_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_WIFI_PASS_KEY, wifi_password, &wifi_pass_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded WiFi password from NVS (length: %zu)", wifi_pass_len);
        } else {
            strncpy(wifi_password, DEFAULT_WIFI_PASSWORD, WIFI_PASS_MAX_LEN - 1);
            wifi_password[WIFI_PASS_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No WiFi password in NVS, using default");
        }

        // Auto Rotate
        uint8_t stored_enabled = 0;
        if (nvs_get_u8(nvs_handle, NVS_AUTO_ROTATE_KEY, &stored_enabled) == ESP_OK) {
            auto_rotate_enabled = (stored_enabled != 0);
            ESP_LOGI(TAG, "Loaded auto-rotate enabled from NVS: %s",
                     auto_rotate_enabled ? "yes" : "no");
        }

        // Rotation schedule (cron). If absent, fall back to migrating a legacy
        // interval, else seed the default — both handled after this handle closes.
        size_t cron_len = sizeof(cron_buf);
        if (nvs_get_str(nvs_handle, NVS_ROTATE_CRON_KEY, cron_buf, &cron_len) == ESP_OK) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            if (!load_cron_valid(cron_buf)) config_read_result(&guard, ESP_ERR_INVALID_RESPONSE);
#endif
            cron_load_from_joined(cron_buf);
            seed_default_cron = false;
            ESP_LOGI(TAG, "Loaded %d cron rule(s) from NVS", cron_rule_count);
        } else if (nvs_get_i32(nvs_handle, NVS_ROTATE_INTERVAL_KEY, &legacy_interval) == ESP_OK) {
            migrate_legacy_interval = true;
            seed_default_cron = false;
        }

        uint8_t stored_mode = ROTATION_MODE_URL;  // Default fallback
        if (nvs_get_u8(nvs_handle, NVS_ROTATION_MODE_KEY, &stored_mode) == ESP_OK) {
            rotation_mode = (rotation_mode_t) stored_mode;
            ESP_LOGI(TAG, "Loaded rotation mode from NVS: %s",
                     rotation_mode == ROTATION_MODE_URL ? "url" : "storage");
        }
#if CONFIG_BOARD_DRIVER_READ_PICO
        else {
            rotation_mode = ROTATION_MODE_STORAGE;
            ESP_LOGI(TAG, "No rotation mode in NVS, using required Read Pico SD storage");
        }
#else
        else if (storage_has_persistent_storage()) {
            rotation_mode = ROTATION_MODE_STORAGE;
            ESP_LOGI(TAG, "No rotation mode in NVS, using default for persistent storage: storage");
        } else {
            ESP_LOGI(TAG, "No rotation mode in NVS, using default for no-storage: url");
        }
#endif

        // Auto Rotate - SDCARD
        uint8_t stored_sd_mode = SD_ROTATION_RANDOM;
        if (nvs_get_u8(nvs_handle, NVS_SD_ROTATION_MODE_KEY, &stored_sd_mode) == ESP_OK) {
            sd_rotation_mode = (sd_rotation_mode_t) stored_sd_mode;
            ESP_LOGI(TAG, "Loaded SD rotation mode from NVS: %s",
                     sd_rotation_mode == SD_ROTATION_SEQUENTIAL ? "sequential" : "random");
        }

        int32_t stored_last_index = -1;
        if (nvs_get_i32(nvs_handle, NVS_LAST_INDEX_KEY, &stored_last_index) == ESP_OK) {
            last_index = stored_last_index;
            ESP_LOGI(TAG, "Loaded last index from NVS: %ld", (long) last_index);
        }

        // Auto Rotate - URL
        size_t url_len = IMAGE_URL_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_IMAGE_URL_KEY, image_url, &url_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded image URL from NVS: %s", image_url);
        } else {
            strncpy(image_url, DEFAULT_IMAGE_URL, IMAGE_URL_MAX_LEN - 1);
            image_url[IMAGE_URL_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No image URL in NVS, using default: %s", image_url);
        }

        // CA Certificate DER blob (heap-allocated)
        size_t blob_len = 0;
        if (nvs_get_blob(nvs_handle, NVS_CA_CERT_KEY, NULL, &blob_len) == ESP_OK && blob_len > 0) {
            ca_cert_der = malloc(blob_len);
#if CONFIG_BOARD_DRIVER_READ_PICO
            if (!ca_cert_der) config_read_result(&guard, ESP_ERR_NO_MEM);
#endif
            if (ca_cert_der &&
                nvs_get_blob(nvs_handle, NVS_CA_CERT_KEY, ca_cert_der, &blob_len) == ESP_OK) {
                ca_cert_der_len = blob_len;
                ESP_LOGI(TAG, "Loaded CA certificate from NVS (%zu bytes)", ca_cert_der_len);
            } else {
                free(ca_cert_der);
                ca_cert_der = NULL;
                ca_cert_der_len = 0;
            }
        }

        size_t access_token_len = ACCESS_TOKEN_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_ACCESS_TOKEN_KEY, access_token, &access_token_len) ==
            ESP_OK) {
            ESP_LOGI(TAG, "Loaded access token from NVS (length: %zu)", access_token_len);
        }

        size_t http_header_key_len = HTTP_HEADER_KEY_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_HTTP_HEADER_KEY_KEY, http_header_key,
                        &http_header_key_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded HTTP header key from NVS: %s", http_header_key);
        }

        size_t http_header_value_len = HTTP_HEADER_VALUE_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_HTTP_HEADER_VALUE_KEY, http_header_value,
                        &http_header_value_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded HTTP header value from NVS (length: %zu)", http_header_value_len);
        }

        uint8_t stored_save_dl = 0;
        if (nvs_get_u8(nvs_handle, NVS_SAVE_DOWNLOADED_KEY, &stored_save_dl) == ESP_OK) {
            save_downloaded_images = (stored_save_dl != 0);
            ESP_LOGI(TAG, "Loaded save_downloaded_images from NVS: %s",
                     save_downloaded_images ? "yes" : "no");
        }

        size_t etag_len = HTTP_ETAG_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_IMAGE_ETAG_KEY, image_etag, &etag_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded image ETag from NVS (length: %zu)", etag_len);
        }

        // Home Assistant
        size_t ha_url_len = HA_URL_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_HA_URL_KEY, ha_url, &ha_url_len) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded HA URL from NVS: %s", ha_url);
        } else {
            strncpy(ha_url, DEFAULT_HA_URL, HA_URL_MAX_LEN - 1);
            ha_url[HA_URL_MAX_LEN - 1] = '\0';
            ESP_LOGI(TAG, "No HA URL in NVS, using default (empty)");
        }

        // AI API Keys
        size_t openai_key_len = AI_API_KEY_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_OPENAI_API_KEY_KEY, openai_api_key, &openai_key_len) ==
            ESP_OK) {
            ESP_LOGI(TAG, "Loaded OpenAI API Key from NVS");
        }

        size_t google_key_len = AI_API_KEY_MAX_LEN;
        if (nvs_get_str(nvs_handle, NVS_GOOGLE_API_KEY_KEY, google_api_key, &google_key_len) ==
            ESP_OK) {
            ESP_LOGI(TAG, "Loaded Google API Key from NVS");
        }

        // Power
        uint8_t deep_sleep_val = 1;  // Default to enabled
        if (nvs_get_u8(nvs_handle, NVS_DEEP_SLEEP_KEY, &deep_sleep_val) == ESP_OK) {
            deep_sleep_enabled = (deep_sleep_val != 0);
            ESP_LOGI(TAG, "Loaded deep sleep setting from NVS: %s",
                     deep_sleep_enabled ? "enabled" : "disabled");
        }

        // Debugging
        uint8_t debug_log_val = 0;
        if (nvs_get_u8(nvs_handle, NVS_DEBUG_LOG_KEY, &debug_log_val) == ESP_OK) {
            debug_log_enabled = (debug_log_val != 0);
            ESP_LOGI(TAG, "Loaded debug log setting from NVS: %s",
                     debug_log_enabled ? "enabled" : "disabled");
        }

        // Config sync timestamp
        if (nvs_get_i64(nvs_handle, "cfg_updated", &config_last_updated) == ESP_OK) {
            ESP_LOGI(TAG, "Loaded config_last_updated: %lld", (long long) config_last_updated);
        }

        nvs_close(nvs_handle);
    }
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (guard.error) {
        free(ca_cert_der); ca_cert_der = NULL; ca_cert_der_len = 0;
        ESP_LOGE(TAG, "Configuration load refused (%s); no migration or key deletion",
                 esp_err_to_name(guard.error));
        return guard.error;
    }
#endif

    // Resolve the rotation schedule now that the read-only handle is closed.
    if (migrate_legacy_interval) {
        char rule[CRON_RULE_MAX_LEN];
        cron_from_legacy_interval((int) legacy_interval, rule, sizeof(rule));
#if CONFIG_BOARD_DRIVER_READ_PICO
        cron_load_from_joined(rule); // Preview in RAM, never persist a boot-time migration.
#else
        const char *one[1] = {rule};
        config_manager_set_cron_rules(one, 1);  // persists to NVS
#endif
#if CONFIG_BOARD_DRIVER_READ_PICO
        ESP_LOGI(TAG, "Legacy interval preview %d s -> cron \"%s\"; stored keys unchanged",
                 (int) legacy_interval, rule);
#else
        ESP_LOGI(TAG, "Migrated legacy interval %d s -> cron \"%s\"", (int) legacy_interval, rule);
#endif
    } else if (seed_default_cron) {
        // Fresh device: seed default in memory; persists on the first user save.
        cron_load_from_joined(DEFAULT_ROTATE_CRON);
        ESP_LOGI(TAG, "No rotation schedule in NVS, using default: %s", DEFAULT_ROTATE_CRON);
    }

    // Erase the legacy quiet-hours (sleep schedule) keys. That feature was
    // replaced by cron rules that carry their own active-hours window; the
    // firmware no longer reads these keys, so drop them from NVS.
#if !CONFIG_BOARD_DRIVER_READ_PICO
    {
        nvs_handle_t erase_handle;
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &erase_handle) == ESP_OK) {
            bool erased = false;
            const char *legacy_keys[] = {
                NVS_SLEEP_SCHEDULE_ENABLED_KEY,
                NVS_SLEEP_SCHEDULE_START_KEY,
                NVS_SLEEP_SCHEDULE_END_KEY,
            };
            for (size_t i = 0; i < sizeof(legacy_keys) / sizeof(legacy_keys[0]); i++) {
                if (nvs_erase_key(erase_handle, legacy_keys[i]) == ESP_OK) {
                    erased = true;
                }
            }
            if (erased) {
                nvs_commit(erase_handle);
                ESP_LOGI(TAG, "Erased legacy sleep-schedule keys from NVS");
            }
            nvs_close(erase_handle);
        }
    }
#endif

    // Apply timezone setting
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t applied = apply_loaded_timezone();
    if (applied != ESP_OK) return applied;
#else
    setenv("TZ", tz_string, 1);
    tzset();
#endif
    ESP_LOGI(TAG, "Timezone set to: %s", tz_string);

    // Log current system time in local timezone
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

    // Calculate UTC offset for display
    struct tm utc_timeinfo;
    gmtime_r(&now, &utc_timeinfo);
    int offset_hours = timeinfo.tm_hour - utc_timeinfo.tm_hour;

    // Handle day boundary crossing
    if (offset_hours > 12)
        offset_hours -= 24;
    if (offset_hours < -12)
        offset_hours += 24;

    ESP_LOGI(TAG, "Config manager initialized");
    return ESP_OK;
}
#if CONFIG_BOARD_DRIVER_READ_PICO
#undef nvs_get_str
#undef nvs_get_u8
#undef nvs_get_i32
#undef nvs_get_i64
#undef nvs_get_blob
#endif

esp_err_t config_manager_init(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    char previous[NTP_SERVER_MAX_LEN];
    char previous_timezone[TIMEZONE_MAX_LEN];
    int64_t previous_revision = config_last_updated;
    char previous_name[DEVICE_NAME_MAX_LEN];
    bool previous_name_confirmed = device_hostnames_confirmed;
    wifi_network_t previous_network;
    memcpy(previous_name, device_name, sizeof(previous_name));
    copy_network_locked(&previous_network);
    memcpy(previous, ntp_server, sizeof(previous));
    memcpy(previous_timezone, tz_string, sizeof(previous_timezone));
    esp_err_t err = config_manager_load();
    // Startup failures do not publish a partially loaded NTP value. Other
    // caches retain their existing startup-only/fail-closed contract.
    if (err != ESP_OK) {
        memcpy(ntp_server, previous, sizeof(previous));
        memcpy(tz_string, previous_timezone, sizeof(previous_timezone));
        config_last_updated = previous_revision;
        memcpy(device_name, previous_name, sizeof(previous_name));
        device_hostnames_confirmed = previous_name_confirmed;
        publish_network_locked(&previous_network);
        // If application was attempted and failed, keep its false flag. A
        // failed loader is not an approved rollback of the global environment.
    }
    atomic_flag_clear(&time_config_control);
    return err;
#else
    return config_manager_load();
#endif
}
// ============================================================================
// General
// ============================================================================

void config_manager_set_device_name(const char *name)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t err = config_manager_set_device_name_checked(name);
    if (err != ESP_OK) ESP_LOGE(TAG, "Device name save unconfirmed: %s", esp_err_to_name(err));
#else
    if (name == NULL) {
        return;
    }

    strncpy(device_name, name, DEVICE_NAME_MAX_LEN - 1);
    device_name[DEVICE_NAME_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_DEVICE_NAME_KEY, device_name);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Device name set to: %s", device_name);
#endif
}
esp_err_t config_manager_set_device_name_checked(const char *name)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    size_t length;
    esp_err_t err = validate_device_name(name, &length);
    if (err == ESP_OK) {
        char candidate[DEVICE_NAME_MAX_LEN] = {0};
        memcpy(candidate, name, length + 1);
        err = store_device_name_locked(candidate);
    }
    atomic_flag_clear(&time_config_control);
    return err;
#else
    if (!name || !*name) return ESP_ERR_INVALID_ARG;
    if (strnlen(name, sizeof(device_name)) >= sizeof(device_name)) return ESP_ERR_INVALID_SIZE;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    err = nvs_set_str(handle, NVS_DEVICE_NAME_KEY, name);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) memcpy(device_name, name, strlen(name) + 1);
    return err;
#endif
}
const char *config_manager_get_device_name(void)
{
    return device_name;
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t config_manager_apply_device_name_checked(const char *name, const device_name_ops_t *ops,
                                                   device_name_result_t *result)
{
    if (!ops || !ops->mdns || !ops->wifi || !result) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    device_name_result_t state = {false, false, DEVICE_NAME_SAVE};
    size_t length;
    esp_err_t err = validate_device_name(name, &length);
    if (err != ESP_OK) goto finished;
    char candidate[DEVICE_NAME_MAX_LEN] = {0};
    memcpy(candidate, name, length + 1);
    const char *current = device_name[0] ? device_name : DEFAULT_DEVICE_NAME;
    if (strcmp(candidate, current)) {
        err = store_device_name_locked(candidate);
        if (err != ESP_OK) goto finished;
    }
    state.name_accepted = true; // Existing loaded/current value needs no rewrite.
    if (!device_hostnames_confirmed) {
        state.step = DEVICE_NAME_MDNS;
        err = ops->mdns(ops->context, candidate);
        if (err != ESP_OK) goto finished;
        state.step = DEVICE_NAME_WIFI;
        err = ops->wifi(ops->context, candidate);
        if (err != ESP_OK) goto finished;
        device_hostnames_confirmed = true;
    }
    state.hostnames_confirmed = true;
    state.step = DEVICE_NAME_COMPLETE;
finished:
    *result = state;
    atomic_flag_clear(&time_config_control);
    return err;
}

esp_err_t config_manager_copy_device_name(char *out, size_t capacity)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    const char *value = device_name[0] ? device_name : DEFAULT_DEVICE_NAME;
    size_t length = strlen(value);
    esp_err_t err = capacity > length ? ESP_OK : ESP_ERR_INVALID_SIZE;
    if (err == ESP_OK) memcpy(out, value, length + 1);
    atomic_flag_clear(&time_config_control);
    return err;
}
esp_err_t config_manager_copy_network(wifi_network_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    copy_network_locked(out);
    atomic_flag_clear(&time_config_control);
    return ESP_OK;
}
esp_err_t config_manager_patch_network_checked(uint32_t fields, const wifi_network_t *patch)
{
    if (!patch || fields & ~WIFI_NETWORK_PATCH_ALL) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    wifi_network_t current, candidate;
    copy_network_locked(&current);
    esp_err_t err = ESP_ERR_INVALID_ARG;
    if (!wifi_network_patch(&candidate, &current, fields, patch)) goto finished;
    err = ESP_OK;
    if (!fields) goto finished;
    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) goto finished;
    const char *keys[] = {NVS_STATIC_IP_KEY, NVS_STATIC_NETMASK_KEY, NVS_STATIC_GATEWAY_KEY, NVS_DNS_SERVER_KEY};
    const char *values[] = {candidate.ip, candidate.netmask, candidate.gateway, candidate.dns};
    for (unsigned i = 0; err == ESP_OK && i < 4; ++i) err = nvs_set_str(handle, keys[i], values[i]);
    if (err == ESP_OK) err = nvs_set_u8(handle, NVS_IP_MODE_KEY, candidate.static_address ? IP_MODE_STATIC : IP_MODE_DHCP);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) publish_network_locked(&candidate);
finished:
    atomic_flag_clear(&time_config_control);
    return err;
}
#endif

esp_err_t config_manager_set_timezone(const char *tz)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    size_t length;
    esp_err_t err = validate_timezone(tz, &length);
    if (err == ESP_OK) {
        char candidate[TIMEZONE_MAX_LEN] = {0};
        memcpy(candidate, tz, length + 1);
        nvs_handle_t handle;
        err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
        if (err == ESP_OK) {
            err = nvs_set_str(handle, NVS_TIMEZONE_KEY, candidate);
            if (err == ESP_OK) err = nvs_commit(handle);
            nvs_close(handle);
        }
        if (err == ESP_OK) {
            memcpy(tz_string, candidate, sizeof(tz_string));
            // Persisted but application may fail; callers must report this
            // partial outcome, not claim saved/runtime state was rolled back.
            err = apply_loaded_timezone();
        }
    }
    atomic_flag_clear(&time_config_control);
    return err;
#else
    if (tz == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    // Refuse an over-long rule rather than store a truncated prefix: a POSIX
    // rule keeps its DST transitions at the end, so the prefix is a
    // different zone that nothing would flag.
    if (strlen(tz) >= TIMEZONE_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    strcpy(tz_string, tz);  // length checked above

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_TIMEZONE_KEY, tz_string);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Timezone set to: %s", tz_string);
    return ESP_OK;
#endif
}
const char *config_manager_get_timezone(void)
{
    if (tz_string[0] == '\0') {
        return "UTC0";
    }
    return tz_string;
}

void config_manager_set_ntp_server(const char *server)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t err = config_manager_set_ntp_server_checked(server);
    if (err != ESP_OK) ESP_LOGE(TAG, "NTP save not confirmed: %s", esp_err_to_name(err));
#else
    if (server == NULL) {
        return;
    }

    strncpy(ntp_server, server, NTP_SERVER_MAX_LEN - 1);
    ntp_server[NTP_SERVER_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_NTP_SERVER_KEY, ntp_server);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "NTP server set to: %s", ntp_server);
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t config_manager_copy_timezone(char *out, size_t capacity, bool *applied)
{
    if (!out || !applied) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    const char *value = tz_string[0] ? tz_string : DEFAULT_TIMEZONE;
    size_t length = strlen(value);
    esp_err_t err = capacity > length ? ESP_OK : ESP_ERR_INVALID_SIZE;
    if (err == ESP_OK) {
        memcpy(out, value, length + 1);
        *applied = timezone_applied;
    }
    atomic_flag_clear(&time_config_control);
    return err;
}

esp_err_t config_manager_copy_local_time(time_t *timestamp, char *formatted, size_t formatted_capacity,
                                        char *timezone, size_t capacity)
{
    if (!timestamp || !formatted || !timezone) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    const char *value = tz_string[0] ? tz_string : DEFAULT_TIMEZONE;
    size_t length = strlen(value);
    esp_err_t err = ESP_ERR_NOT_FINISHED;
    if (capacity <= length || !formatted_capacity) err = ESP_ERR_INVALID_SIZE;
    else if (timezone_applied) {
        time_t now;
        struct tm candidate;
        char text[64];
        err = ESP_ERR_INVALID_STATE;
        if (time(&now) != (time_t)-1 && now >= 1577836800 && localtime_r(&now, &candidate) &&
            strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &candidate)) {
            size_t text_length = strlen(text);
            if (text_length >= formatted_capacity) {
                atomic_flag_clear(&time_config_control);
                return ESP_ERR_INVALID_SIZE;
            }
            *timestamp = now;
            memcpy(formatted, text, text_length + 1);
            memcpy(timezone, value, length + 1);
            err = ESP_OK;
        }
    }
    atomic_flag_clear(&time_config_control);
    return err;
}

esp_err_t config_manager_set_ntp_server_checked(const char *server)
{
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    size_t length;
    esp_err_t err = validate_ntp_server(server, &length);
    if (err != ESP_OK) {
        atomic_flag_clear(&time_config_control);
        return err;
    }
    // Snapshot before IO; a caller-owned request is not retained by NVS/cache.
    char candidate[NTP_SERVER_MAX_LEN] = {0};
    memcpy(candidate, server, length + 1);
    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, NVS_NTP_SERVER_KEY, candidate);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }
    if (err == ESP_OK) memcpy(ntp_server, candidate, sizeof(ntp_server));
    atomic_flag_clear(&time_config_control);
    // A failed set/commit may still have affected storage; no rollback claim.
    return err;
}

esp_err_t config_manager_copy_ntp_server(char *out, size_t capacity)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    const char *value = ntp_server[0] ? ntp_server : DEFAULT_NTP_SERVER;
    size_t length = strlen(value);
    esp_err_t err = capacity > length ? ESP_OK : ESP_ERR_INVALID_SIZE;
    if (err == ESP_OK) memcpy(out, value, length + 1);
    atomic_flag_clear(&time_config_control);
    return err;
}
#endif

const char *config_manager_get_ntp_server(void)
{
    if (ntp_server[0] == '\0') {
        return DEFAULT_NTP_SERVER;
    }
    return ntp_server;
}

// ----------------------------------------------------------------------------
// Advanced network settings
// ----------------------------------------------------------------------------

// Persist one dotted-IPv4 string setting (helper for the network settings).
static void set_ip_str(const char *nvs_key, char *cache, const char *value)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    (void)cache;
    wifi_network_t patch = {0};
    uint32_t fields;
    char *destination;
    if (!strcmp(nvs_key, NVS_STATIC_IP_KEY)) { fields = WIFI_NETWORK_PATCH_IP; destination = patch.ip; }
    else if (!strcmp(nvs_key, NVS_STATIC_NETMASK_KEY)) { fields = WIFI_NETWORK_PATCH_NETMASK; destination = patch.netmask; }
    else if (!strcmp(nvs_key, NVS_STATIC_GATEWAY_KEY)) { fields = WIFI_NETWORK_PATCH_GATEWAY; destination = patch.gateway; }
    else { fields = WIFI_NETWORK_PATCH_DNS; destination = patch.dns; }
    if (!value) value = "";
    size_t length = strnlen(value, 16);
    if (length == 16) { ESP_LOGE(TAG, "Network value too long; not saved"); return; }
    memcpy(destination, value, length + 1);
    esp_err_t err = config_manager_patch_network_checked(fields, &patch);
    if (err != ESP_OK) ESP_LOGE(TAG, "Network value save unconfirmed: %s", esp_err_to_name(err));
#else
    if (value == NULL) {
        value = "";
    }
    strncpy(cache, value, IP_ADDR_STR_MAX_LEN - 1);
    cache[IP_ADDR_STR_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, nvs_key, cache);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
#endif
}

void config_manager_set_ip_mode(ip_mode_t mode)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (mode != IP_MODE_DHCP && mode != IP_MODE_STATIC) { ESP_LOGE(TAG, "Invalid IP mode; not saved"); return; }
    wifi_network_t patch = {.static_address = mode == IP_MODE_STATIC};
    esp_err_t err = config_manager_patch_network_checked(WIFI_NETWORK_PATCH_MODE, &patch);
    if (err != ESP_OK) ESP_LOGE(TAG, "IP mode save unconfirmed: %s", esp_err_to_name(err));
#else
    ip_mode = mode;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_IP_MODE_KEY, (uint8_t) mode);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    ESP_LOGI(TAG, "IP mode set to: %s", mode == IP_MODE_STATIC ? "static" : "dhcp");
#endif
}

ip_mode_t config_manager_get_ip_mode(void)
{
    return ip_mode;
}

void config_manager_set_static_ip(const char *ip)
{
    set_ip_str(NVS_STATIC_IP_KEY, static_ip, ip);
}

const char *config_manager_get_static_ip(void)
{
    return static_ip;
}

void config_manager_set_static_netmask(const char *mask)
{
    set_ip_str(NVS_STATIC_NETMASK_KEY, static_netmask, mask);
}

const char *config_manager_get_static_netmask(void)
{
    return static_netmask;
}

void config_manager_set_static_gateway(const char *gw)
{
    set_ip_str(NVS_STATIC_GATEWAY_KEY, static_gateway, gw);
}

const char *config_manager_get_static_gateway(void)
{
    return static_gateway;
}

void config_manager_set_dns_server(const char *dns)
{
    set_ip_str(NVS_DNS_SERVER_KEY, dns_server, dns);
}

const char *config_manager_get_dns_server(void)
{
    return dns_server;
}

esp_err_t config_manager_save_provisioning_checked(const char *ssid, const char *password,
                                                  const char *name, const wifi_network_t *network)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    size_t name_length;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    if (!wifi_text_credentials_valid(ssid, password) ||
        validate_device_name(name, &name_length) != ESP_OK || !wifi_network_valid(network)) {
        atomic_flag_clear(&time_config_control);
        return ESP_ERR_INVALID_ARG;
    }
    wifi_text_t strings = {0};
    memcpy(strings.ssid, ssid, strlen(ssid) + 1);
    memcpy(strings.password, password, strlen(password) + 1);
    memcpy(strings.device_name, name, strlen(name) + 1);
    wifi_network_t settings = *network;
    // One handle/commit reduces intermediate published state, but NVS set calls
    // may already have persisted. Never claim rollback or multi-key atomicity.
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        atomic_flag_clear(&time_config_control);
        return err;
    }
    const char *keys[] = {NVS_WIFI_SSID_KEY, NVS_WIFI_PASS_KEY, NVS_DEVICE_NAME_KEY,
                         NVS_STATIC_IP_KEY, NVS_STATIC_NETMASK_KEY, NVS_STATIC_GATEWAY_KEY,
                         NVS_DNS_SERVER_KEY};
    const char *values[] = {strings.ssid, strings.password, strings.device_name, settings.ip,
                           settings.netmask, settings.gateway, settings.dns};
    for (unsigned i = 0; err == ESP_OK && i < sizeof(keys) / sizeof(keys[0]); ++i)
        err = nvs_set_str(handle, keys[i], values[i]);
    if (err == ESP_OK) err = nvs_set_u8(handle, NVS_IP_MODE_KEY,
                                      settings.static_address ? IP_MODE_STATIC : IP_MODE_DHCP);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    if (err == ESP_OK) {
        memcpy(wifi_ssid, strings.ssid, sizeof(wifi_ssid));
        memcpy(wifi_password, strings.password, sizeof(wifi_password));
        memcpy(device_name, strings.device_name, sizeof(device_name));
        device_hostnames_confirmed = false;
        publish_network_locked(&settings);
    }
    atomic_flag_clear(&time_config_control);
    return err;
#else
    (void)ssid; (void)password; (void)name; (void)network;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

void config_manager_set_display_orientation(display_orientation_t orientation)
{
    display_orientation = orientation;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_DISPLAY_ORIENTATION_KEY, (uint8_t) orientation);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Display orientation set to: %s",
             orientation == DISPLAY_ORIENTATION_LANDSCAPE ? "landscape" : "portrait");
}

display_orientation_t config_manager_get_display_orientation(void)
{
    return display_orientation;
}

void config_manager_set_display_rotation_deg(int rotation_deg)
{
    display_rotation_deg = rotation_deg;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_i32(nvs_handle, NVS_DISPLAY_ROTATION_DEG_KEY, rotation_deg);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Display rotation set to %d degrees", rotation_deg);
}

int config_manager_get_display_rotation_deg(void)
{
    return display_rotation_deg;
}

void config_manager_set_wifi_ssid(const char *ssid)
{
    if (ssid == NULL) {
        return;
    }

    strncpy(wifi_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    wifi_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_WIFI_SSID_KEY, wifi_ssid);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "WiFi SSID set to: %s", wifi_ssid);
}

const char *config_manager_get_wifi_ssid(void)
{
    return wifi_ssid;
}

void config_manager_set_wifi_password(const char *password)
{
    if (password == NULL) {
        return;
    }

    strncpy(wifi_password, password, WIFI_PASS_MAX_LEN - 1);
    wifi_password[WIFI_PASS_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_WIFI_PASS_KEY, wifi_password);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "WiFi password set (length: %zu)", strlen(wifi_password));
}

const char *config_manager_get_wifi_password(void)
{
    return wifi_password;
}
// ============================================================================
// Auto Rotate
// ============================================================================

void config_manager_set_auto_rotate(bool enabled)
{
    auto_rotate_enabled = enabled;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_AUTO_ROTATE_KEY, enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Auto-rotate %s", enabled ? "enabled" : "disabled");
}

bool config_manager_get_auto_rotate(void)
{
    return auto_rotate_enabled;
}

int config_manager_get_cron_rule_count(void)
{
    return cron_rule_count;
}

const char *config_manager_get_cron_rule(int index)
{
    if (index < 0 || index >= cron_rule_count) {
        return NULL;
    }
    return cron_rules_store[index];
}

void config_manager_set_cron_rules(const char *const *rules, int count)
{
    if (count < 0) {
        count = 0;
    }
    cron_rule_count = 0;
    for (int i = 0; i < count && cron_rule_count < MAX_CRON_RULES; i++) {
        if (!rules[i] || rules[i][0] == '\0' || strlen(rules[i]) >= CRON_RULE_MAX_LEN) {
            continue;
        }
        strncpy(cron_rules_store[cron_rule_count], rules[i], CRON_RULE_MAX_LEN - 1);
        cron_rules_store[cron_rule_count][CRON_RULE_MAX_LEN - 1] = '\0';
        cron_rule_count++;
    }

    cron_persist();
    ESP_LOGI(TAG, "Rotation schedule set to %d cron rule(s)", cron_rule_count);
}

void config_manager_set_cron_rules_from_interval(int seconds)
{
    char rule[CRON_RULE_MAX_LEN];
    cron_from_legacy_interval(seconds, rule, sizeof(rule));
    const char *one[1] = {rule};
    config_manager_set_cron_rules(one, 1);
}

int config_manager_get_compiled_cron_rules(cron_rule_t *out, int max)
{
    int n = 0;
    for (int i = 0; i < cron_rule_count && n < max; i++) {
        if (cron_parse(cron_rules_store[i], &out[n])) {
            n++;
        }
    }
    return n;
}

void config_manager_set_rotation_mode(rotation_mode_t mode)
{
    if (!storage_has_persistent_storage() && mode == ROTATION_MODE_STORAGE) {
        ESP_LOGE(TAG, "Cannot set rotation mode to STORAGE: Local storage not supported");
        return;
    }

    rotation_mode = mode;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_ROTATION_MODE_KEY, (uint8_t) mode);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Rotation mode set to: %s", mode == ROTATION_MODE_URL ? "url" : "sdcard");
}

rotation_mode_t config_manager_get_rotation_mode(void)
{
    return rotation_mode;
}
// ============================================================================
// Auto Rotate - SDCARD
// ============================================================================

void config_manager_set_sd_rotation_mode(sd_rotation_mode_t mode)
{
    sd_rotation_mode = mode;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_SD_ROTATION_MODE_KEY, (uint8_t) mode);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "SD rotation mode set to: %s",
             mode == SD_ROTATION_SEQUENTIAL ? "sequential" : "random");
}

sd_rotation_mode_t config_manager_get_sd_rotation_mode(void)
{
    return sd_rotation_mode;
}

void config_manager_set_last_index(int32_t index)
{
    last_index = index;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_i32(nvs_handle, NVS_LAST_INDEX_KEY, index);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

int32_t config_manager_get_last_index(void)
{
    return last_index;
}

// ============================================================================
// Auto Rotate - URL
// ============================================================================

void config_manager_set_image_url(const char *url)
{
    const char *new_url = url ? url : "";
    bool url_changed = strcmp(image_url, new_url) != 0;

    strncpy(image_url, new_url, IMAGE_URL_MAX_LEN - 1);
    image_url[IMAGE_URL_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        if (image_url[0] != '\0') {
            nvs_set_str(nvs_handle, NVS_IMAGE_URL_KEY, image_url);
        } else {
            nvs_erase_key(nvs_handle, NVS_IMAGE_URL_KEY);
        }
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    if (url_changed) {
        config_manager_set_image_etag("");
    }

    ESP_LOGI(TAG, "Image URL set to: %s", image_url[0] ? image_url : "(empty)");
}
const char *config_manager_get_image_url(void)
{
    return image_url;
}

void config_manager_set_ca_cert_der(const uint8_t *der, size_t len)
{
    free(ca_cert_der);
    ca_cert_der = NULL;
    ca_cert_der_len = 0;

    if (der && len > 0) {
        ca_cert_der = malloc(len);
        if (ca_cert_der) {
            memcpy(ca_cert_der, der, len);
            ca_cert_der_len = len;
        }
    }

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        if (ca_cert_der) {
            nvs_set_blob(nvs_handle, NVS_CA_CERT_KEY, ca_cert_der, ca_cert_der_len);
        } else {
            nvs_erase_key(nvs_handle, NVS_CA_CERT_KEY);
        }
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "CA certificate %s (%zu bytes)", ca_cert_der ? "set" : "cleared",
             ca_cert_der_len);
}

const uint8_t *config_manager_get_ca_cert_der(size_t *out_len)
{
    if (out_len) {
        *out_len = ca_cert_der_len;
    }
    return ca_cert_der;
}

void config_manager_set_access_token(const char *token)
{
    if (token == NULL) {
        return;
    }

    strncpy(access_token, token, ACCESS_TOKEN_MAX_LEN - 1);
    access_token[ACCESS_TOKEN_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_ACCESS_TOKEN_KEY, access_token);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Access token set (length: %zu)", strlen(access_token));
}
const char *config_manager_get_access_token(void)
{
    return access_token;
}

esp_err_t config_manager_set_http_password(const char *password)
{
    if (password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(password) >= HTTP_PASSWORD_MAX_LEN) {
        return ESP_ERR_INVALID_SIZE;
    }

    // Persist first and only then switch the live value, so a failed write
    // can't leave a password that guards this boot but vanishes on the next.
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs_handle, NVS_HTTP_PASSWORD_KEY, password);
        if (err == ESP_OK) {
            err = nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store HTTP API password: %s", esp_err_to_name(err));
        return err;
    }

    strcpy(http_password, password);  // length checked above

    // Never log the value itself.
    ESP_LOGI(TAG, "HTTP API password %s", http_password[0] ? "set" : "cleared");
    return ESP_OK;
}

const char *config_manager_get_http_password(void)
{
    return http_password;
}

void config_manager_set_http_header_key(const char *key)
{
    if (key == NULL) {
        return;
    }

    strncpy(http_header_key, key, HTTP_HEADER_KEY_MAX_LEN - 1);
    http_header_key[HTTP_HEADER_KEY_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_HTTP_HEADER_KEY_KEY, http_header_key);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "HTTP header key set to: %s", http_header_key);
}
const char *config_manager_get_http_header_key(void)
{
    return http_header_key;
}

void config_manager_set_http_header_value(const char *value)
{
    if (value == NULL) {
        return;
    }

    strncpy(http_header_value, value, HTTP_HEADER_VALUE_MAX_LEN - 1);
    http_header_value[HTTP_HEADER_VALUE_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_HTTP_HEADER_VALUE_KEY, http_header_value);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "HTTP header value set (length: %zu)", strlen(http_header_value));
}
const char *config_manager_get_http_header_value(void)
{
    return http_header_value;
}

void config_manager_set_save_downloaded_images(bool enabled)
{
    save_downloaded_images = enabled;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_SAVE_DOWNLOADED_KEY, enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Save downloaded images %s", enabled ? "enabled" : "disabled");
}

bool config_manager_get_save_downloaded_images(void)
{
    return save_downloaded_images;
}

void config_manager_set_image_etag(const char *etag)
{
    const char *new_etag = etag ? etag : "";

    // No-op if value unchanged — avoids NVS wear when the server does not send
    // an ETag (empty stays empty across every 200) or repeats the same ETag.
    if (strncmp(image_etag, new_etag, HTTP_ETAG_MAX_LEN) == 0) {
        return;
    }

    strncpy(image_etag, new_etag, HTTP_ETAG_MAX_LEN - 1);
    image_etag[HTTP_ETAG_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        if (image_etag[0] != '\0') {
            nvs_set_str(nvs_handle, NVS_IMAGE_ETAG_KEY, image_etag);
        } else {
            nvs_erase_key(nvs_handle, NVS_IMAGE_ETAG_KEY);
        }
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
}

const char *config_manager_get_image_etag(void)
{
    return image_etag;
}
// ============================================================================
// Home Assistant
// ============================================================================

void config_manager_set_ha_url(const char *url)
{
    if (url) {
        strncpy(ha_url, url, HA_URL_MAX_LEN - 1);
        ha_url[HA_URL_MAX_LEN - 1] = '\0';

        // Strip trailing slashes so callers can safely append "/api/...".
        // A doubled slash ("host//api/...") is a different path to Home
        // Assistant's router and returns 404.
        size_t len = strlen(ha_url);
        while (len > 0 && ha_url[len - 1] == '/') {
            ha_url[--len] = '\0';
        }

        nvs_handle_t nvs_handle;
        if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
            nvs_set_str(nvs_handle, NVS_HA_URL_KEY, ha_url);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
        }

        ESP_LOGI(TAG, "HA URL set to: %s", ha_url);
    }
}
const char *config_manager_get_ha_url(void)
{
    return ha_url;
}

// ============================================================================
// AI Generation
// ============================================================================

void config_manager_set_openai_api_key(const char *key)
{
    if (key == NULL) {
        return;
    }

    strncpy(openai_api_key, key, AI_API_KEY_MAX_LEN - 1);
    openai_api_key[AI_API_KEY_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_OPENAI_API_KEY_KEY, openai_api_key);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "OpenAI API Key set");
}

const char *config_manager_get_openai_api_key(void)
{
    return openai_api_key;
}

void config_manager_set_google_api_key(const char *key)
{
    if (key == NULL) {
        return;
    }

    strncpy(google_api_key, key, AI_API_KEY_MAX_LEN - 1);
    google_api_key[AI_API_KEY_MAX_LEN - 1] = '\0';

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_str(nvs_handle, NVS_GOOGLE_API_KEY_KEY, google_api_key);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Google API Key set");
}

const char *config_manager_get_google_api_key(void)
{
    return google_api_key;
}

void config_manager_set_deep_sleep_enabled(bool enabled)
{
    deep_sleep_enabled = enabled;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_DEEP_SLEEP_KEY, enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Deep sleep %s", enabled ? "enabled" : "disabled");
}

bool config_manager_get_deep_sleep_enabled(void)
{
    return deep_sleep_enabled;
}

void config_manager_set_debug_log_enabled(bool enabled)
{
    debug_log_enabled = enabled;

    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_u8(nvs_handle, NVS_DEBUG_LOG_KEY, enabled ? 1 : 0);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    ESP_LOGI(TAG, "Debug log %s", enabled ? "enabled" : "disabled");
}

bool config_manager_get_debug_log_enabled(void)
{
    return debug_log_enabled;
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t store_config_revision(int64_t candidate)
{
    if (candidate < 1577836800 || candidate < config_last_updated) return ESP_ERR_INVALID_ARG;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_i64(handle, "cfg_updated", candidate);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }
    if (err == ESP_OK) config_last_updated = candidate;
    return err;
}
#endif

void config_manager_set_config_last_updated(int64_t timestamp)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (atomic_flag_test_and_set(&time_config_control)) {
        ESP_LOGE(TAG, "Config revision busy; not saved");
        return;
    }
    esp_err_t err = store_config_revision(timestamp);
    atomic_flag_clear(&time_config_control);
    if (err != ESP_OK) ESP_LOGE(TAG, "Config revision save unconfirmed: %s", esp_err_to_name(err));
#else
    config_last_updated = timestamp;
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_set_i64(nvs_handle, "cfg_updated", config_last_updated);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
#endif
}

int64_t config_manager_get_config_last_updated(void)
{
    return config_last_updated;
}

void config_manager_touch_config(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t err = config_manager_touch_config_checked();
    if (err != ESP_OK) ESP_LOGE(TAG, "Config revision not confirmed: %s", esp_err_to_name(err));
#else
    time_t now;
    time(&now);
    config_manager_set_config_last_updated((int64_t) now);
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t config_manager_copy_config_last_updated(int64_t *timestamp)
{
    if (!timestamp) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    *timestamp = config_last_updated;
    atomic_flag_clear(&time_config_control);
    return ESP_OK;
}

esp_err_t config_manager_touch_config_checked(void)
{
    if (atomic_flag_test_and_set(&time_config_control)) return ESP_ERR_INVALID_STATE;
    time_t now;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (time(&now) == (time_t)-1 || now < 1577836800 || config_last_updated == INT64_MAX) goto done;
    int64_t candidate = (int64_t)now;
    if (candidate <= config_last_updated) candidate = config_last_updated + 1;
    err = store_config_revision(candidate);
done:
    atomic_flag_clear(&time_config_control);
    return err;
}
#endif
