#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

#include "esp_err.h"
#include "wifi_credentials.h"
#include "wifi_network.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Read Pico: repeated successful initialization reuses the same resources;
// clean pre-start failure may be explicitly retried. Uncertain SDK ownership
// or runtime cleanup failure rejects reinitialization; no automatic recovery.
esp_err_t wifi_manager_init(void);
// Read Pico: stable manager-owned AP interface only after checked initialization.
// This is not a reservation against all other radio consumers.
esp_netif_t *wifi_manager_provisioning_netif(void);
// Read Pico coordination with the original provisioning service. begin/quiesce
// reserve the existing connection mutex for this call; finish releases it in
// the same task, never across returned service calls. STOPPED is only valid
// after the caller has confirmed all acquired resources exited. A failed
// service retains its non-reused owner ID and blocks new station work.
// quiesce cancels retries first; busy returns without granting SDK control.
typedef enum { WIFI_PROVISION_READY, WIFI_PROVISION_STOPPED, WIFI_PROVISION_FAILED }
    wifi_provisioning_outcome_t;
esp_err_t wifi_manager_provisioning_begin(uint32_t *owner, esp_netif_t **netif);
esp_err_t wifi_manager_provisioning_quiesce(uint32_t owner);
esp_err_t wifi_manager_provisioning_finish(uint32_t owner, wifi_provisioning_outcome_t outcome);
esp_err_t wifi_manager_update_hostname(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t wifi_manager_update_hostname_named(const char *name);
#endif
// Toggle between full-RX performance (WIFI_PS_NONE, low latency / fast web UI)
// and modem power save (WIFI_PS_MIN_MODEM). Idempotent; safe to call every
// second. The policy for when to use which lives in power_manager.
esp_err_t wifi_manager_set_performance_mode(bool enable);
// Apply the configured IP mode to the STA netif (static address or DHCP).
// Called automatically by wifi_manager_connect; exposed for the provisioning
// legacy provisioning connection test; Read Pico uses the checked manager.
esp_err_t wifi_manager_apply_ip_config(void);
// Connect and wait for an IP, bounded by a time limit. ESP_OK once connected;
// ESP_FAIL when every retry failed, or when the time limit ran out while the
// AP was rejecting the credentials; ESP_ERR_TIMEOUT when the time limit ran
// out for any other reason (DHCP not answering, AP absent or slow). On a
// timeout the attempt is still running: follow up with
// wifi_manager_stop_connecting() or wifi_manager_keep_reconnecting().
esp_err_t wifi_manager_connect(const char *ssid, const char *password);
// Checked Read Pico candidate connection, preserving the provisioning AP.
esp_err_t wifi_manager_test_credentials(const char *ssid, const char *password);
// Explicit temporary network settings, without changing config manager/NVS.
esp_err_t wifi_manager_test_network(const char *ssid, const char *password,
                                    const wifi_network_t *network);
// Owner is published only on successful test. Cleanup checks this attempt's
// non-reused ID, so a failed/late handler cannot stop a newer candidate.
esp_err_t wifi_manager_test_network_owned(const char *ssid, const char *password,
                                          const wifi_network_t *network, uint32_t *owner);
esp_err_t wifi_manager_cancel_candidate(uint32_t owner);
// Give up on the current connection attempt: no more automatic reconnects,
// and WiFi is stopped. For callers that will not use the network this wake.
void wifi_manager_stop_connecting(void);
// Read Pico: report stop failure, disable uncertain radio reuse; never stops
// a provisioning-owned AP or takes control away from an in-flight operation.
esp_err_t wifi_manager_stop_connecting_checked(void);
// Keep reconnecting with no retry limit until connected (or until sleep stops
// WiFi). For callers that stay awake and want the network whenever it appears.
// The one exception is an AP that keeps rejecting the credentials: after as
// many rejections as a normal connect allows, WIFI_FAIL_BIT is set and the
// retries stop.
void wifi_manager_keep_reconnecting(void);
esp_err_t wifi_manager_disconnect(void);
bool wifi_manager_is_connected(void);
// Read Pico returns a complete current address only for a verified live link;
// caller output stays unchanged on stale state, read or capacity failures.
esp_err_t wifi_manager_get_ip(char *ip_str, size_t len);
esp_err_t wifi_manager_load_credentials_from_sdcard(char *ssid, char *password);
EventGroupHandle_t wifi_manager_get_event_group(void);
int wifi_manager_scan(wifi_ap_record_t *results, int max_results);
// Read Pico provisioning-only scan: idle AP, serialized against candidate tests.
// On failure both outputs are unchanged; ESP_OK with count=0 means truly empty.
#define WIFI_MANAGER_SCAN_LIMIT 20
esp_err_t wifi_manager_scan_checked(wifi_ap_record_t *results, int max_results, int *count);

#endif
