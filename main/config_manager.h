#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "config.h"
#include "cron.h"
#include "esp_err.h"
#include "wifi_network.h"

esp_err_t config_manager_init(void);

// ============================================================================
// General
// ============================================================================

void config_manager_set_device_name(const char *name);
// Persist/sync the explicit import before changing the live value.
esp_err_t config_manager_set_device_name_checked(const char *name);
const char *config_manager_get_device_name(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t config_manager_copy_device_name(char *out, size_t capacity);
typedef enum { DEVICE_NAME_SAVE, DEVICE_NAME_MDNS, DEVICE_NAME_WIFI, DEVICE_NAME_COMPLETE } device_name_step_t;
typedef struct {
    void *context;
    esp_err_t (*mdns)(void *context, const char *name);
    esp_err_t (*wifi)(void *context, const char *name);
} device_name_ops_t;
typedef struct { bool name_accepted; bool hostnames_confirmed; device_name_step_t step; } device_name_result_t;
// Same-value requests retry unconfirmed side effects without another NVS write.
// Callbacks receive one owned name while checked configuration is busy. They
// must not use checked config getters/setters recursively; those return busy.
// Confirmation is volatile and means callback returns, not RF advertisement.
// name_accepted includes an unchanged cached/default value; it is not proof
// that the value has a physical NVS record. Busy/invalid ops leave result alone.
esp_err_t config_manager_apply_device_name_checked(const char *name, const device_name_ops_t *ops,
                                                   device_name_result_t *result);
#endif

esp_err_t config_manager_set_timezone(const char *tz);
const char *config_manager_get_timezone(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
// Setter checks persistence, setenv/readback and calls tzset (void, no grammar
// result). A
// NOT_FINISHED result means saved RAM may have changed, application unconfirmed.
// Copies serialize with timezone/NTP operations; outputs unchanged on error.
esp_err_t config_manager_copy_timezone(char *out, size_t capacity, bool *applied);
esp_err_t config_manager_copy_local_time(time_t *timestamp, char *formatted, size_t formatted_capacity,
                                        char *timezone, size_t capacity);
#endif

void config_manager_set_ntp_server(const char *server);
const char *config_manager_get_ntp_server(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
// Checked NTP cache access. Empty input retains the upstream default fallback.
// Copy output is unchanged on error; set publishes RAM only after commit.
// The borrowed getter above remains unsafe for concurrent runtime consumers.
esp_err_t config_manager_set_ntp_server_checked(const char *server);
esp_err_t config_manager_copy_ntp_server(char *out, size_t capacity);
#endif

// Advanced network settings (#43), same collapsed UI section as the NTP server
// above: static IP (instead of DHCP) and DNS override. The DNS override applies
// in both IP modes (empty string = automatic). Values are dotted IPv4 strings.
void config_manager_set_ip_mode(ip_mode_t mode);
ip_mode_t config_manager_get_ip_mode(void);
void config_manager_set_static_ip(const char *ip);
const char *config_manager_get_static_ip(void);
void config_manager_set_static_netmask(const char *mask);
const char *config_manager_get_static_netmask(void);
void config_manager_set_static_gateway(const char *gw);
const char *config_manager_get_static_gateway(void);
void config_manager_set_dns_server(const char *dns);
const char *config_manager_get_dns_server(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
// Raw, owned cached snapshot; malformed retained static fields can be repaired
// by a complete patch. Persistence publishes the full group only on commit.
esp_err_t config_manager_copy_network(wifi_network_t *out);
esp_err_t config_manager_patch_network_checked(uint32_t fields, const wifi_network_t *patch);
#endif
// Read Pico explicit post-test save. Every set and the single commit is checked;
// RAM publishes only after success. NVS multi-key failure can still be partial.
esp_err_t config_manager_save_provisioning_checked(const char *ssid, const char *password,
                                                  const char *name, const wifi_network_t *network);

void config_manager_set_display_orientation(display_orientation_t orientation);
display_orientation_t config_manager_get_display_orientation(void);

void config_manager_set_display_rotation_deg(int rotation_deg);
int config_manager_get_display_rotation_deg(void);

void config_manager_set_wifi_ssid(const char *ssid);
const char *config_manager_get_wifi_ssid(void);

void config_manager_set_wifi_password(const char *password);
const char *config_manager_get_wifi_password(void);

// ============================================================================
// Auto Rotate
// ============================================================================

void config_manager_set_auto_rotate(bool enabled);
bool config_manager_get_auto_rotate(void);

int config_manager_get_cron_rule_count(void);

// Returns NULL if index is out of range.
const char *config_manager_get_cron_rule(int index);

// Replaces the rule set (caps at MAX_CRON_RULES, drops empty/over-long entries);
// caller should validate expressions with cron_parse() first.
void config_manager_set_cron_rules(const char *const *rules, int count);
void config_manager_set_cron_rules_from_interval(int seconds);

// Compiles the stored rules into `out`; returns the count written (<= max),
// skipping any that fail to parse.
int config_manager_get_compiled_cron_rules(cron_rule_t *out, int max);

void config_manager_set_rotation_mode(rotation_mode_t mode);
rotation_mode_t config_manager_get_rotation_mode(void);

// ============================================================================
// Auto Rotate - SDCARD
// ============================================================================

void config_manager_set_sd_rotation_mode(sd_rotation_mode_t mode);
sd_rotation_mode_t config_manager_get_sd_rotation_mode(void);

void config_manager_set_last_index(int32_t index);
int32_t config_manager_get_last_index(void);

// ============================================================================
// Auto Rotate - URL
// ============================================================================

void config_manager_set_image_url(const char *url);
const char *config_manager_get_image_url(void);

void config_manager_set_ca_cert_der(const uint8_t *der, size_t len);
const uint8_t *config_manager_get_ca_cert_der(size_t *out_len);

void config_manager_set_access_token(const char *token);
const char *config_manager_get_access_token(void);

/**
 * @brief Set the password guarding the device's own HTTP API. Empty disables
 *        authentication, which is the default.
 * @return ESP_OK once stored; ESP_ERR_INVALID_SIZE if longer than
 *         HTTP_PASSWORD_MAX_LEN - 1 bytes; an NVS error if it could not be
 *         persisted, in which case the previous password stays in effect.
 */
esp_err_t config_manager_set_http_password(const char *password);

/** @brief Current HTTP API password; empty string when authentication is off. */
const char *config_manager_get_http_password(void);

void config_manager_set_http_header_key(const char *key);
const char *config_manager_get_http_header_key(void);

void config_manager_set_http_header_value(const char *value);
const char *config_manager_get_http_header_value(void);

void config_manager_set_save_downloaded_images(bool enabled);
bool config_manager_get_save_downloaded_images(void);

// ETag captured from the last successful image fetch.
// Empty string means "not set" — skip the If-None-Match request header.
void config_manager_set_image_etag(const char *etag);
const char *config_manager_get_image_etag(void);

#if CONFIG_BOARD_DRIVER_READ_PICO
typedef struct {
    char url[IMAGE_URL_MAX_LEN];
    char token[ACCESS_TOKEN_MAX_LEN];
    char header_key[HTTP_HEADER_KEY_MAX_LEN];
    char header_value[HTTP_HEADER_VALUE_MAX_LEN];
    char etag[HTTP_ETAG_MAX_LEN];
    uint8_t *certificate;
    size_t certificate_length;
} download_config_t;
enum {
    DOWNLOAD_CONFIG_URL = 1u << 0,
    DOWNLOAD_CONFIG_TOKEN = 1u << 1,
    DOWNLOAD_CONFIG_HEADER_KEY = 1u << 2,
    DOWNLOAD_CONFIG_HEADER_VALUE = 1u << 3,
    DOWNLOAD_CONFIG_CERTIFICATE = 1u << 4,
    DOWNLOAD_CONFIG_ETAG = 1u << 5,
};
// Owned cache snapshot under the same control as all checked/compatibility
// writers and startup loading. Output is unchanged on error; release before
// recapturing into a previously successful snapshot.
esp_err_t config_manager_copy_download(download_config_t *out);
void config_manager_release_download(download_config_t *snapshot);
// Selected fields are copied before IO. RAM publishes only after all writes
// and one commit succeed. NVS can still be partially changed on failure.
// Source/credential changes also invalidate ETag in that commit. ETag cannot
// be patched together with source/credential fields.
esp_err_t config_manager_patch_download_checked(uint32_t fields, const download_config_t *patch);
esp_err_t config_manager_set_image_source_checked(const char *url, const uint8_t *der, size_t length);
// Accept an ETag only if the request's source/credentials still match. A
// changed source returns OK without storing; storage/control errors propagate.
esp_err_t config_manager_accept_download_etag(const download_config_t *request, const char *etag);
#endif

// ============================================================================
// Home Assistant
// ============================================================================

void config_manager_set_ha_url(const char *url);
const char *config_manager_get_ha_url(void);

// ============================================================================
// AI API Keys
// ============================================================================

void config_manager_set_openai_api_key(const char *key);
const char *config_manager_get_openai_api_key(void);

void config_manager_set_google_api_key(const char *key);
const char *config_manager_get_google_api_key(void);

// ============================================================================
// Power
// ============================================================================

void config_manager_set_deep_sleep_enabled(bool enabled);
bool config_manager_get_deep_sleep_enabled(void);

// ============================================================================
// Debugging
// ============================================================================

void config_manager_set_debug_log_enabled(bool enabled);
bool config_manager_get_debug_log_enabled(void);

// ============================================================================
// Config Sync
// ============================================================================

void config_manager_set_config_last_updated(int64_t timestamp);
int64_t config_manager_get_config_last_updated(void);
void config_manager_touch_config(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t config_manager_touch_config_checked(void);
esp_err_t config_manager_copy_config_last_updated(int64_t *timestamp);
#endif

#endif
