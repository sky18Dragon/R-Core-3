#include "wifi_manager.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include "wifi_text.h"
#include "wifi_link.h"
#include <stdatomic.h>
#include "esp_wifi_default.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "config_manager.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "storage.h"
#include "utils.h"

static const char *TAG = "wifi_manager";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// Reconnect attempts after the first one before giving up.
#define WIFI_MAX_RETRY 5
// Hard cap on one wifi_manager_connect(). Association plus DHCP normally takes
// a few seconds; this only matters when the AP is out of range or DHCP never
// answers, where waiting longer just burns battery (#121).
#define WIFI_CONNECT_TIMEOUT_MS 30000

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_is_connected = false;
// Reconnect policy, shared between wifi_manager_connect()'s caller and the
// event handler (which runs on the event-loop task), so guarded by
// s_policy_lock. A mutex rather than a spinlock: the handler must decide to
// stop retrying and publish WIFI_FAIL_BIT as one step, and event-group calls
// are not allowed inside a critical section.
//  - s_give_up: wifi_manager_stop_connecting() was called; never reconnect.
//  - s_keep_trying: wifi_manager_keep_reconnecting() was called; reconnect
//    without the WIFI_MAX_RETRY limit.
//  - s_retries_exhausted: the handler stopped reconnecting and set
//    WIFI_FAIL_BIT, so nothing is in progress any more.
static SemaphoreHandle_t s_policy_lock = NULL;
static bool s_give_up = false;
static bool s_keep_trying = false;
static bool s_retries_exhausted = false;
// Consecutive attempts the AP rejected in a way that points at the password
// (see is_auth_rejection()). Reset whenever the AP accepts us or an attempt
// fails for some other reason, so only an unbroken run of rejections counts.
static int s_auth_rejects = 0;  // guarded by s_policy_lock too

// Disconnect reasons that mean the AP turned the credentials down, as opposed
// to it being absent or out of range (NO_AP_FOUND, BEACON_TIMEOUT, ...). A
// wrong WPA2 passphrase surfaces as a 4-way handshake timeout.
static bool is_auth_rejection(uint8_t reason)
{
    return reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || reason == WIFI_REASON_AUTH_FAIL ||
           reason == WIFI_REASON_HANDSHAKE_TIMEOUT;
}
static esp_netif_t *s_sta_netif = NULL;
#if CONFIG_BOARD_DRIVER_READ_PICO
ESP_EVENT_DEFINE_BASE(READ_PICO_WIFI_FENCE);
#define WIFI_FENCE_BIT BIT2
static SemaphoreHandle_t s_connection_lock;
static wifi_link_t s_link;
static bool s_candidate;
static unsigned s_fence_serial;
static atomic_bool s_driver_ready;
static esp_err_t s_link_error;
static wifi_network_t s_network;
static atomic_flag s_init_busy = ATOMIC_FLAG_INIT;
static bool s_initialized, s_init_uncertain, s_owns_driver;
static esp_netif_t *s_ap_netif;
static bool s_sta_attached, s_ap_attached;
static esp_event_handler_instance_t s_instances[5];
enum { RADIO_IDLE, RADIO_AP_STARTING, RADIO_AP_ACTIVE, RADIO_AP_STOPPING, RADIO_AP_FAILED };
static atomic_int s_radio_phase = RADIO_IDLE;
static uint32_t s_ap_serial, s_ap_owner; // Under policy lock; never reused after wrap.
static bool s_ap_control; // Only AP begin/quiesce owns the connection mutex.
static TaskHandle_t s_ap_task;

static bool station_phase_allowed(bool candidate)
{
    return s_radio_phase == RADIO_IDLE || (candidate && s_radio_phase == RADIO_AP_ACTIVE);
}
static esp_err_t take_radio_operation(bool candidate)
{
    if (!s_driver_ready || !s_connection_lock || !s_policy_lock ||
        xSemaphoreTake(s_connection_lock, 0) != pdTRUE) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    bool allowed = station_phase_allowed(candidate);
    xSemaphoreGive(s_policy_lock);
    if (!allowed) { xSemaphoreGive(s_connection_lock); return ESP_ERR_INVALID_STATE; }
    return ESP_OK;
}

static esp_event_base_t init_event_base(unsigned index)
{
    return index == 0 ? WIFI_EVENT : index == 3 ? READ_PICO_WIFI_FENCE : IP_EVENT;
}
static int32_t init_event_id(unsigned index)
{
    static const int32_t ids[] = {ESP_EVENT_ANY_ID, IP_EVENT_STA_GOT_IP,
                                 IP_EVENT_GOT_IP6, ESP_EVENT_ANY_ID, IP_EVENT_STA_LOST_IP};
    return ids[index];
}

// Only used before manager publication: no radio start or business consumers.
static esp_err_t cleanup_initialization(void)
{
    for (unsigned i = 5; i-- > 0;) {
        if (!s_instances[i]) continue;
        esp_err_t err = esp_event_handler_instance_unregister(init_event_base(i), init_event_id(i),
                                                              s_instances[i]);
        if (err != ESP_OK) return err;
        s_instances[i] = NULL;
    }
    if (s_owns_driver) {
        esp_err_t err = esp_wifi_deinit();
        if (err != ESP_OK) return err;
        s_owns_driver = false;
    }
    esp_netif_t **interfaces[] = {&s_ap_netif, &s_sta_netif};
    bool *attached[] = {&s_ap_attached, &s_sta_attached};
    for (unsigned i = 0; i < 2; ++i) {
        if (!*interfaces[i]) continue;
        if (*attached[i]) {
            esp_err_t err = esp_wifi_clear_default_wifi_driver_and_handlers(*interfaces[i]);
            // SDK may already destroy the interface driver on error; never retry it.
            if (err != ESP_OK) return err;
            *attached[i] = false;
        }
        esp_netif_destroy(*interfaces[i]);
        *interfaces[i] = NULL;
    }
    return ESP_OK;
}

static bool current_link_matches(void)
{
    wifi_ap_record_t ap;
    esp_netif_ip_info_t ip;
    if (!s_sta_netif || !esp_netif_is_netif_up(s_sta_netif) ||
        esp_wifi_sta_get_ap_info(&ap) != ESP_OK || esp_netif_get_ip_info(s_sta_netif, &ip) != ESP_OK ||
        !ip.ip.addr) return false;
    return wifi_link_ip(&s_link, ap.ssid, strnlen((char *)ap.ssid, sizeof(ap.ssid)), ap.bssid, true);
}
static void cancel_link_locked(void)
{
    wifi_link_cancel(&s_link); s_is_connected = false;
    memset(&s_network, 0, sizeof(s_network));
    s_give_up = true; s_keep_trying = false;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
}
#endif

#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t apply_dns_override(void);
#else
static void apply_dns_override(void);
#endif

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                          void *event_data)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!s_driver_ready) return;
    if (event_base == READ_PICO_WIFI_FENCE) {
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        unsigned serial = 0;
        if (event_data) memcpy(&serial, event_data, sizeof(serial)); // Private fixed-size posted message.
        if (event_data && serial == s_fence_serial)
            xEventGroupSetBits(s_wifi_event_group, WIFI_FENCE_BIT);
        xSemaphoreGive(s_policy_lock); return;
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        s_is_connected = false; wifi_link_disconnected(&s_link);
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xSemaphoreGive(s_policy_lock); return;
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        s_link.ready = s_is_connected = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xSemaphoreGive(s_policy_lock); return;
    }
#endif
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
#if CONFIG_BOARD_DRIVER_READ_PICO
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        if (station_phase_allowed(s_candidate) && s_link.active && !s_give_up && !s_candidate) {
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                s_link_error = err; s_retries_exhausted = true;
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
        xSemaphoreGive(s_policy_lock);
#else
        esp_wifi_connect();
#endif
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        // Bring up an IPv6 link-local address so mDNS can answer AAAA queries.
        // Without one the responder stays silent on AAAA, and clients resolving
        // <name>.local wait out their full resolver timeout (~5s per request)
        // before falling back to the A record.
#if !CONFIG_BOARD_DRIVER_READ_PICO
        esp_netif_create_ip6_linklocal(s_sta_netif);
#endif
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
#if CONFIG_BOARD_DRIVER_READ_PICO
        const wifi_event_sta_connected_t *connected = event_data;
        wifi_ap_record_t ap;
        bool matching = station_phase_allowed(s_candidate) && connected && connected->ssid_len <= sizeof(connected->ssid) &&
            esp_wifi_sta_get_ap_info(&ap) == ESP_OK &&
            connected->ssid_len == strnlen((char *)ap.ssid, sizeof(ap.ssid)) &&
            !memcmp(connected->ssid, ap.ssid, connected->ssid_len) &&
            !memcmp(connected->bssid, ap.bssid, 6) &&
            wifi_link_associated(&s_link, connected->ssid, connected->ssid_len, connected->bssid);
        if (!matching) { xSemaphoreGive(s_policy_lock); return; }
        esp_netif_create_ip6_linklocal(s_sta_netif);
#endif
        s_auth_rejects = 0;  // the AP accepted the credentials
        xSemaphoreGive(s_policy_lock);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disc =
            (const wifi_event_sta_disconnected_t *) event_data;
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
#if CONFIG_BOARD_DRIVER_READ_PICO
        wifi_link_disconnected(&s_link); s_is_connected = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (!s_link.active || !station_phase_allowed(s_candidate)) { xSemaphoreGive(s_policy_lock); return; }
#endif
        if (disc && is_auth_rejection(disc->reason)) {
            s_auth_rejects++;
        } else {
            s_auth_rejects = 0;
        }
        // Background reconnects (s_keep_trying) wait out an absent AP or a
        // silent DHCP server indefinitely, but not an AP that keeps refusing
        // the password: that gets the same number of attempts as a normal
        // connect, then WIFI_FAIL_BIT so the owner of the retry can react.
        bool retry = !s_give_up && (s_keep_trying ? s_auth_rejects <= WIFI_MAX_RETRY
                                                  : s_retry_num < WIFI_MAX_RETRY);
        if (retry) {
            s_retry_num++;
#if CONFIG_BOARD_DRIVER_READ_PICO
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                s_link_error = err; s_retries_exhausted = true;
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
#else
            esp_wifi_connect();
#endif
        } else {
            s_retries_exhausted = true;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        xSemaphoreGive(s_policy_lock);
        ESP_LOGI(TAG, "%s", retry ? "retry to connect to the AP" : "giving up on the AP");
#if !CONFIG_BOARD_DRIVER_READ_PICO
        s_is_connected = false;
        // Waiters on WIFI_CONNECTED_BIT (e.g. main.c's late_wifi_task) must
        // see the link as down again, not a stale bit from an earlier IP.
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
#endif
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
#if CONFIG_BOARD_DRIVER_READ_PICO
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        esp_netif_ip_info_t actual;
        if (!station_phase_allowed(s_candidate) || !event || event->esp_netif != s_sta_netif ||
            esp_netif_get_ip_info(s_sta_netif, &actual) != ESP_OK ||
            actual.ip.addr != event->ip_info.ip.addr || !current_link_matches()) {
            xSemaphoreGive(s_policy_lock); return;
        }
#endif
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        // Applied after the address is up so it overrides DHCP-provided DNS
        // servers too (#43).
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t dns = apply_dns_override();
        if (dns != ESP_OK) {
            s_link_error = dns; s_link.ready = s_is_connected = false;
            s_give_up = s_retries_exhausted = true;
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            xSemaphoreGive(s_policy_lock); return;
        }
        s_link_error = ESP_OK;
#else
        apply_dns_override();
#endif
        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
#if CONFIG_BOARD_DRIVER_READ_PICO
        s_retries_exhausted = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
        xSemaphoreGive(s_policy_lock);
#endif
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *) event_data;
        ESP_LOGI(TAG, "got ip6:" IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

esp_err_t wifi_manager_set_performance_mode(bool enable)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    // Reconnect/setup also changes PS. Do not trust a function-local cache
    // that outlives those driver changes or masks a later SDK failure.
    if (take_radio_operation(true) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t err = esp_wifi_set_ps(enable ? WIFI_PS_NONE : WIFI_PS_MIN_MODEM);
    xSemaphoreGive(s_connection_lock);
    return err;
#else
    // Modem power save adds ~100ms+ of latency to every round trip, which
    // throttles the web UI hard: bulk transfer speed is roughly one TCP send
    // buffer (~5.7KB) per round trip, i.e. ~45KB/s at 130ms RTT. Full RX
    // (WIFI_PS_NONE) costs ~60-70mA extra while the radio is up, so it is only
    // enabled when someone may actually be using the UI — the policy lives in
    // power_manager's sleep_timer_task.
    static bool applied = false;
    static bool current = false;
    if (applied && current == enable) {
        return ESP_OK;
    }

    esp_err_t err = esp_wifi_set_ps(enable ? WIFI_PS_NONE : WIFI_PS_MIN_MODEM);
    if (err == ESP_OK) {
        applied = true;
        current = enable;
        ESP_LOGI(TAG, "WiFi power save %s (%s mode)", enable ? "disabled" : "enabled",
                 enable ? "performance" : "power-save");
    }
    return err;
#endif
}

static esp_err_t apply_hostname_value(const char *name)
{
    if (!s_sta_netif) {
        return ESP_ERR_INVALID_STATE;
    }

    // DHCP hostname from the device name (CamelCase, shown in router device
    // lists). The router picks it up at the next DHCP negotiation (reconnect).
    char hostname[64];
    sanitize_dhcp_hostname(name, hostname, sizeof(hostname));
    esp_err_t err = esp_netif_set_hostname(s_sta_netif, hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set DHCP hostname: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "DHCP hostname set to: %s", hostname);
    return ESP_OK;
}
static esp_err_t apply_hostname(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    char name[DEVICE_NAME_MAX_LEN];
    esp_err_t copied = config_manager_copy_device_name(name, sizeof(name));
    if (copied != ESP_OK) return copied;
    return apply_hostname_value(name);
#else
    return apply_hostname_value(config_manager_get_device_name());
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t wifi_manager_update_hostname_named(const char *name)
{
    if (!name || !*name) return ESP_ERR_INVALID_ARG;
    size_t length = strnlen(name, DEVICE_NAME_MAX_LEN);
    if (length == DEVICE_NAME_MAX_LEN) return ESP_ERR_INVALID_SIZE;
    char snapshot[DEVICE_NAME_MAX_LEN] = {0};
    memcpy(snapshot, name, length + 1);
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)snapshot[i] < 0x20 || (unsigned char)snapshot[i] == 0x7f) return ESP_ERR_INVALID_ARG;
    if (take_radio_operation(true) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t err = apply_hostname_value(snapshot);
    xSemaphoreGive(s_connection_lock);
    return err;
}
#endif

esp_err_t wifi_manager_update_hostname(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (take_radio_operation(true) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t error = apply_hostname();
    xSemaphoreGive(s_connection_lock);
    return error;
#else
    return apply_hostname();
#endif
}

esp_err_t wifi_manager_init(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (atomic_flag_test_and_set(&s_init_busy)) return ESP_ERR_INVALID_STATE;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (s_init_uncertain) goto finished;
    if (s_initialized) {
        err = s_driver_ready &&
            (s_radio_phase == RADIO_IDLE || s_radio_phase == RADIO_AP_ACTIVE) ? ESP_OK : ESP_ERR_INVALID_STATE;
        goto finished;
    }
    // Stable, bounded application-lifetime synchronization objects: retries do
    // not replace handles that another consumer might retain.
    if (!s_wifi_event_group) s_wifi_event_group = xEventGroupCreate();
    if (!s_policy_lock) s_policy_lock = xSemaphoreCreateMutex();
    if (!s_connection_lock) s_connection_lock = xSemaphoreCreateMutex();
    if (!s_wifi_event_group || !s_policy_lock || !s_connection_lock) {
        err = ESP_ERR_NO_MEM; goto finished;
    }
    wifi_link_cancel(&s_link); s_give_up = true; s_keep_trying = false;
    err = esp_netif_init();
    if (err != ESP_OK) goto failed;
    err = esp_event_loop_create_default();
    // The default event loop/netif core are shared, application-lifetime SDK
    // infrastructure. Never delete them on a local WiFi initialization error.
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) goto failed;
    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") ||
        esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")) {
        err = ESP_ERR_INVALID_STATE; goto failed; // Do not take another owner's interfaces.
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 0;
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        // No proven driver ownership after failed SDK init; do not deinit a
        // foreign driver or repeatedly enter its unknown partial state.
        s_init_uncertain = true; goto failed;
    }
    s_owns_driver = true;
    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) goto failed;
    esp_netif_config_t sta_config = ESP_NETIF_DEFAULT_WIFI_STA();
    s_sta_netif = esp_netif_new(&sta_config);
    if (!s_sta_netif) { err = ESP_ERR_NO_MEM; goto failed; }
    err = esp_netif_attach_wifi_station(s_sta_netif);
    if (err != ESP_OK) { s_init_uncertain = true; goto failed; }
    s_sta_attached = true;
    esp_netif_config_t ap_config = ESP_NETIF_DEFAULT_WIFI_AP();
    s_ap_netif = esp_netif_new(&ap_config);
    if (!s_ap_netif) { err = ESP_ERR_NO_MEM; goto failed; }
    err = esp_netif_attach_wifi_ap(s_ap_netif);
    if (err != ESP_OK) { s_init_uncertain = true; goto failed; }
    s_ap_attached = true;
    err = esp_wifi_set_default_wifi_sta_handlers();
    if (err != ESP_OK) goto failed;
    err = apply_hostname();
    if (err != ESP_OK) goto failed;
    for (unsigned i = 0; i < 5; ++i) {
        err = esp_event_handler_instance_register(init_event_base(i), init_event_id(i),
                                                 &event_handler, NULL, &s_instances[i]);
        if (err != ESP_OK) goto failed;
    }
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) goto failed;
    s_initialized = true;
    s_driver_ready = true;
    goto finished;
failed:
    s_driver_ready = false;
    // Failed SDK attach can leak an unpublished driver inside the helper. Its
    // netif/global pointer and other owned objects are retained, not guessed freed.
    if (!s_init_uncertain) {
        esp_err_t cleanup = cleanup_initialization();
        if (cleanup != ESP_OK) {
            s_init_uncertain = true;
            ESP_LOGE(TAG, "WiFi initialization cleanup unconfirmed: %s", esp_err_to_name(cleanup));
        }
    }
    ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(err));
finished:
    atomic_flag_clear(&s_init_busy);
    return err;
#else
    s_wifi_event_group = xEventGroupCreate();
    s_policy_lock = xSemaphoreCreateMutex();
    if (!s_wifi_event_group || !s_policy_lock) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create both STA and AP network interfaces
    s_sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_manager_update_hostname();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_t instance_got_ip6;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                        &event_handler, NULL, &instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_GOT_IP6, &event_handler,
                                                        NULL, &instance_got_ip6));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    // Don't start WiFi here - let wifi_manager_connect() or wifi_provisioning_start_ap() start it
    // ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_manager_init finished.");

    return ESP_OK;
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t configured_network(wifi_network_t *out)
{
    esp_err_t copied = config_manager_copy_network(out);
    if (copied != ESP_OK) return copied;
    return wifi_network_valid(out) ? ESP_OK : ESP_ERR_INVALID_ARG;
}
static esp_err_t apply_ip_network(const wifi_network_t *network)
{
    if (!s_sta_netif) return ESP_ERR_INVALID_STATE;
    if (!wifi_network_valid(network)) return ESP_ERR_INVALID_ARG;
    esp_netif_ip_info_t ip_info = {0};
    bool static_address = network->static_address;
    if (static_address &&
        (esp_netif_str_to_ip4(network->ip, &ip_info.ip) != ESP_OK ||
         esp_netif_str_to_ip4(network->netmask, &ip_info.netmask) != ESP_OK ||
         esp_netif_str_to_ip4(network->gateway, &ip_info.gw) != ESP_OK))
        return ESP_ERR_INVALID_ARG;
    esp_err_t stopped = esp_netif_dhcpc_stop(s_sta_netif);
    if (stopped != ESP_OK && stopped != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) return stopped;
    // Even when DHCP was already stopped, overwrite its retained old address.
    esp_err_t set = esp_netif_set_ip_info(s_sta_netif, &ip_info);
    if (set != ESP_OK || static_address) return set;
    esp_err_t started = esp_netif_dhcpc_start(s_sta_netif);
    return started == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED ? ESP_OK : started;
}
#endif

esp_err_t wifi_manager_apply_ip_config(void)
{
    if (!s_sta_netif) return ESP_ERR_INVALID_STATE;
#if CONFIG_BOARD_DRIVER_READ_PICO
    wifi_network_t network;
    if (take_radio_operation(false) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t error = configured_network(&network);
    if (error == ESP_OK) error = apply_ip_network(&network);
    xSemaphoreGive(s_connection_lock);
    return error;
#else
    if (config_manager_get_ip_mode() == IP_MODE_STATIC) {
        esp_netif_ip_info_t ip_info = {0};
        if (esp_netif_str_to_ip4(config_manager_get_static_ip(), &ip_info.ip) != ESP_OK ||
            esp_netif_str_to_ip4(config_manager_get_static_netmask(), &ip_info.netmask) != ESP_OK ||
            esp_netif_str_to_ip4(config_manager_get_static_gateway(), &ip_info.gw) != ESP_OK) {
            // Never brick the connection on a malformed config — fall back to
            // DHCP so the frame stays reachable and the user can fix it.
            ESP_LOGE(TAG, "Invalid static IP config, falling back to DHCP");
            esp_netif_dhcpc_start(s_sta_netif);
            return ESP_ERR_INVALID_ARG;
        }

        esp_netif_dhcpc_stop(s_sta_netif);
        esp_netif_set_ip_info(s_sta_netif, &ip_info);
        ESP_LOGI(TAG, "Static IP applied: %s/%s gw %s", config_manager_get_static_ip(),
                 config_manager_get_static_netmask(), config_manager_get_static_gateway());
    } else {
        // Make sure DHCP runs when switching back from a static config.
        // Returns ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED in the normal case.
        esp_netif_dhcpc_start(s_sta_netif);
    }
    return ESP_OK;
#endif
}

// Apply the DNS override (if configured). Called after GOT_IP so it takes
// precedence over DHCP-provided servers in DHCP mode; in static mode it is the
// only DNS source (defaults to the gateway when unset).
#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t apply_dns_override(void)
#else
static void apply_dns_override(void)
#endif
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    const char *dns = s_network.dns;
    if (!dns[0] && s_network.static_address) dns = s_network.gateway;
#else
    const char *dns = config_manager_get_dns_server();
    if ((dns == NULL || dns[0] == '\0') && config_manager_get_ip_mode() == IP_MODE_STATIC) {
        dns = config_manager_get_static_gateway();
    }
#endif
    if (dns == NULL || dns[0] == '\0') {
#if CONFIG_BOARD_DRIVER_READ_PICO
        return ESP_OK;
#else
        return;
#endif
    }

    esp_netif_dns_info_t dns_info = {0};
    if (esp_netif_str_to_ip4(dns, &dns_info.ip.u_addr.ip4) != ESP_OK) {
        ESP_LOGE(TAG, "Invalid DNS server: %s", dns);
#if CONFIG_BOARD_DRIVER_READ_PICO
        return ESP_ERR_INVALID_ARG;
#else
        return;
#endif
    }
    dns_info.ip.type = ESP_IPADDR_TYPE_V4;
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t err = esp_netif_set_dns_info(s_sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    if (err != ESP_OK) return err;
#else
    esp_netif_set_dns_info(s_sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
#endif
    ESP_LOGI(TAG, "DNS server set to: %s", dns);
#if CONFIG_BOARD_DRIVER_READ_PICO
    return ESP_OK;
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static esp_err_t checked_connect(const char *ssid, const char *password, bool candidate,
                                 const wifi_network_t *settings, uint32_t *owner)
{
    if (!wifi_text_credentials_valid(ssid, password ? password : "")) return ESP_ERR_INVALID_ARG;
    wifi_network_t network;
    if (settings) {
        if (!wifi_network_valid(settings)) return ESP_ERR_INVALID_ARG;
        network = *settings;
    } else {
        esp_err_t copied = configured_network(&network);
        if (copied != ESP_OK) return copied;
    }
    if (take_radio_operation(candidate) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t err;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (!station_phase_allowed(candidate)) {
        xSemaphoreGive(s_policy_lock); xSemaphoreGive(s_connection_lock);
        return ESP_ERR_INVALID_STATE;
    }
    cancel_link_locked(); s_candidate = candidate;
    if (s_fence_serial == UINT32_MAX) {
        xSemaphoreGive(s_policy_lock); xSemaphoreGive(s_connection_lock); return ESP_ERR_INVALID_STATE;
    }
    unsigned serial = ++s_fence_serial;
    xEventGroupClearBits(s_wifi_event_group, WIFI_FENCE_BIT);
    xSemaphoreGive(s_policy_lock);
    // Candidate setup disables only STA, retaining the AP and this HTTP request.
    err = candidate ? esp_wifi_set_mode(WIFI_MODE_AP) : esp_wifi_stop();
    if (err != ESP_OK && !(err == ESP_ERR_WIFI_NOT_STARTED && !candidate)) goto failed;
    // Fence callbacks already queued on the default event loop. This is not
    // an invented attempt ID on SDK events; current association/IP are also checked.
    err = esp_event_post(READ_PICO_WIFI_FENCE, 0, &serial, sizeof(serial), pdMS_TO_TICKS(1000));
    if (err != ESP_OK) goto failed;
    if (!(xEventGroupWaitBits(s_wifi_event_group, WIFI_FENCE_BIT, pdTRUE, pdFALSE,
                             pdMS_TO_TICKS(1000)) & WIFI_FENCE_BIT)) {
        err = ESP_ERR_TIMEOUT; goto failed;
    }
    err = esp_wifi_set_mode(candidate ? WIFI_MODE_APSTA : WIFI_MODE_STA);
    if (err != ESP_OK) goto failed;
    wifi_config_t config = {0};
    memcpy(config.sta.ssid, ssid, strlen(ssid));
    if (password) memcpy(config.sta.password, password, strlen(password));
    config.sta.threshold.authmode = password && *password ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
    config.sta.pmf_cfg.capable = true; config.sta.pmf_cfg.required = false;
    err = esp_wifi_set_config(WIFI_IF_STA, &config);
    if (err != ESP_OK) goto failed;
    err = apply_ip_network(&network);
    if (err != ESP_OK) goto failed;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (!station_phase_allowed(candidate)) {
        xSemaphoreGive(s_policy_lock); err = ESP_ERR_INVALID_STATE; goto failed;
    }
    wifi_link_arm(&s_link, ssid); s_is_connected = false;
    s_network = network; // Immutable DNS/IP settings for this attempt, under policy lock.
    s_link_error = ESP_OK;
    s_retry_num = s_auth_rejects = 0; s_give_up = s_keep_trying = s_retries_exhausted = false;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    xSemaphoreGive(s_policy_lock);
    // Reset BEFORE starting/connecting, so fast fresh events are not erased.
    err = candidate ? esp_wifi_connect() : esp_wifi_start();
    if (err != ESP_OK) goto failed;
    if (!candidate) {
        err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        if (err != ESP_OK) goto failed;
    }
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(candidate ? 15000 : WIFI_CONNECT_TIMEOUT_MS));
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    bool good = (bits & WIFI_CONNECTED_BIT) && s_is_connected && current_link_matches();
    bool rejected = s_auth_rejects > 0 || (bits & WIFI_FAIL_BIT);
    esp_err_t link_error = s_link_error;
    xSemaphoreGive(s_policy_lock);
    if (good) {
        if (owner) *owner = serial;
        xSemaphoreGive(s_connection_lock); return ESP_OK;
    }
    err = link_error != ESP_OK ? link_error : rejected ? ESP_FAIL : ESP_ERR_TIMEOUT;
    if (!candidate && !rejected) {
        // Keep the established retry/timeout contract for ordinary boot.
        xSemaphoreGive(s_connection_lock); return err;
    }
failed:
    xSemaphoreTake(s_policy_lock, portMAX_DELAY); cancel_link_locked(); xSemaphoreGive(s_policy_lock);
    {
        esp_err_t cleanup = candidate ? esp_wifi_set_mode(WIFI_MODE_AP) : esp_wifi_stop();
        if (cleanup != ESP_OK && cleanup != ESP_ERR_WIFI_NOT_STARTED)
            ESP_LOGE(TAG, "Connection failed (%s), cleanup=%s", esp_err_to_name(err), esp_err_to_name(cleanup));
    }
    xSemaphoreGive(s_connection_lock);
    return err;
}
#endif

esp_netif_t *wifi_manager_provisioning_netif(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    return s_driver_ready &&
        (s_radio_phase == RADIO_IDLE || s_radio_phase == RADIO_AP_ACTIVE) ? s_ap_netif : NULL;
#else
    return NULL;
#endif
}

esp_err_t wifi_manager_provisioning_begin(uint32_t *owner, esp_netif_t **netif)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!owner || !netif) return ESP_ERR_INVALID_ARG;
    if (take_radio_operation(false) != ESP_OK) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (s_ap_owner || s_ap_serial == UINT32_MAX || !s_ap_netif) {
        xSemaphoreGive(s_policy_lock); xSemaphoreGive(s_connection_lock);
        return ESP_ERR_INVALID_STATE;
    }
    cancel_link_locked(); s_candidate = false;
    s_ap_owner = ++s_ap_serial; s_ap_control = true;
    s_ap_task = xTaskGetCurrentTaskHandle(); s_radio_phase = RADIO_AP_STARTING;
    *owner = s_ap_owner; *netif = s_ap_netif;
    xSemaphoreGive(s_policy_lock);
    return ESP_OK; // Caller must finish in this task before returning.
#else
    (void)owner; (void)netif; return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_provisioning_quiesce(uint32_t owner)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!owner || !s_policy_lock || !s_connection_lock) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (owner != s_ap_owner || (s_ap_control && s_ap_task != xTaskGetCurrentTaskHandle())) {
        xSemaphoreGive(s_policy_lock); return ESP_ERR_INVALID_STATE;
    }
    // Cancel BEFORE trying the connection mutex, so an in-flight candidate can
    // wake, finish its own cleanup and release it. No SDK teardown while busy.
    s_radio_phase = RADIO_AP_STOPPING;
    cancel_link_locked(); s_candidate = false;
    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    if (s_ap_control) { xSemaphoreGive(s_policy_lock); return ESP_OK; } // Startup failure.
    if (xSemaphoreTake(s_connection_lock, 0) != pdTRUE) {
        xSemaphoreGive(s_policy_lock); return ESP_ERR_INVALID_STATE;
    }
    s_ap_control = true; s_ap_task = xTaskGetCurrentTaskHandle();
    xSemaphoreGive(s_policy_lock);
    return ESP_OK;
#else
    (void)owner; return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_provisioning_finish(uint32_t owner, wifi_provisioning_outcome_t outcome)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (outcome != WIFI_PROVISION_READY && outcome != WIFI_PROVISION_STOPPED &&
        outcome != WIFI_PROVISION_FAILED) return ESP_ERR_INVALID_ARG;
    if (!owner || !s_policy_lock || !s_connection_lock) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (owner != s_ap_owner ||
        (s_ap_control && s_ap_task != xTaskGetCurrentTaskHandle()) ||
        (outcome != WIFI_PROVISION_FAILED && !s_ap_control) ||
        (outcome == WIFI_PROVISION_READY && s_radio_phase != RADIO_AP_STARTING)) {
        xSemaphoreGive(s_policy_lock); return ESP_ERR_INVALID_STATE;
    }
    bool held = s_ap_control; s_ap_control = false; s_ap_task = NULL;
    s_radio_phase = outcome == WIFI_PROVISION_READY ? RADIO_AP_ACTIVE :
                    outcome == WIFI_PROVISION_STOPPED ? RADIO_IDLE : RADIO_AP_FAILED;
    if (outcome == WIFI_PROVISION_STOPPED) s_ap_owner = 0;
    xSemaphoreGive(s_policy_lock);
    if (held) xSemaphoreGive(s_connection_lock); // Never give a candidate's mutex.
    return ESP_OK;
#else
    (void)owner; (void)outcome; return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_test_credentials(const char *ssid, const char *password)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    return checked_connect(ssid, password, true, NULL, NULL);
#else
    (void)ssid; (void)password; return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_test_network(const char *ssid, const char *password,
                                    const wifi_network_t *network)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!network) return ESP_ERR_INVALID_ARG;
    return checked_connect(ssid, password, true, network, NULL);
#else
    (void)ssid; (void)password; (void)network;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_test_network_owned(const char *ssid, const char *password,
                                          const wifi_network_t *network, uint32_t *owner)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!network || !owner) return ESP_ERR_INVALID_ARG;
    return checked_connect(ssid, password, true, network, owner);
#else
    (void)ssid; (void)password; (void)network; (void)owner;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_cancel_candidate(uint32_t owner)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!owner || take_radio_operation(true) != ESP_OK) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (owner != s_fence_serial || !s_candidate || !s_link.active) {
        xSemaphoreGive(s_policy_lock); xSemaphoreGive(s_connection_lock);
        return ESP_ERR_INVALID_STATE;
    }
    cancel_link_locked(); s_candidate = false;
    xSemaphoreGive(s_policy_lock);
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    xSemaphoreGive(s_connection_lock);
    return err; // A cleanup SDK error is not proof the station is actually off.
#else
    (void)owner; return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    return checked_connect(ssid, password, false, NULL, NULL);
#else
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "SSID is empty");
        return ESP_ERR_INVALID_ARG;
    }

    wifi_manager_apply_ip_config();

    wifi_config_t wifi_config = {0};
    strncpy((char *) wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char *) wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    // Stop WiFi if it's running, then set config
    esp_wifi_stop();

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));  // Enable power save at boot/connect

    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    s_retry_num = 0;
    s_give_up = false;
    s_keep_trying = false;
    s_retries_exhausted = false;
    s_auth_rejects = 0;
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    xSemaphoreGive(s_policy_lock);
    EventBits_t bits =
        xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
                            pdFALSE, pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        wifi_ap_record_t ap;
        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
            ESP_LOGI(TAG, "connected to ap SSID:%s (RSSI %d dBm, channel %d)", ssid, ap.rssi,
                     ap.primary);
        } else {
            ESP_LOGI(TAG, "connected to ap SSID:%s", ssid);
        }
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGW(TAG, "Failed to connect to SSID:%s after %d attempts", ssid, WIFI_MAX_RETRY + 1);
        return ESP_FAIL;
    }

    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    bool rejected = s_auth_rejects > 0;
    xSemaphoreGive(s_policy_lock);
    if (rejected) {
        // Out of time while the AP was still rejecting the password: that is
        // a credential failure that merely ran slow (a wrong passphrase costs
        // a multi-second handshake timeout per attempt), not a slow network.
        // Report it as ESP_FAIL so an interactive boot still falls back to
        // provisioning rather than retrying a wrong password forever.
        ESP_LOGW(TAG, "SSID:%s rejected the credentials; giving up after %d ms", ssid,
                 WIFI_CONNECT_TIMEOUT_MS);
        wifi_manager_stop_connecting();
        return ESP_FAIL;
    }

    // Timed out otherwise: associated but no IP yet (DHCP not answering), or
    // the AP not answering at all (still booting, out of range). Neither says
    // the credentials are wrong, so this is not ESP_FAIL. The attempt is still
    // running; the caller decides whether to stop it or keep it going.
    ESP_LOGW(TAG, "Timed out connecting to SSID:%s after %d ms", ssid, WIFI_CONNECT_TIMEOUT_MS);
    return ESP_ERR_TIMEOUT;
#endif
}

esp_err_t wifi_manager_stop_connecting_checked(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (take_radio_operation(false) != ESP_OK) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    cancel_link_locked(); s_candidate = false;
    xSemaphoreGive(s_policy_lock);
    esp_err_t error = esp_wifi_stop();
    if (error == ESP_ERR_WIFI_NOT_STARTED) error = ESP_OK;
    if (error != ESP_OK) s_driver_ready = false; // No automatic reuse of uncertain stop.
    xSemaphoreGive(s_connection_lock);
    return error;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

void wifi_manager_stop_connecting(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t error = wifi_manager_stop_connecting_checked();
    if (error != ESP_OK) ESP_LOGE(TAG, "WiFi stop unconfirmed: %s", esp_err_to_name(error));
#else
    // Stop the event handler from reconnecting on its own and power the radio
    // down, so it isn't left churning while the caller moves on (#121).
    // esp_wifi_stop() rather than a disconnect: the handler may already have
    // decided to retry and be about to call esp_wifi_connect(), which a
    // disconnect issued first would not cancel; once WiFi is stopped that
    // call simply fails.
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    s_give_up = true;
    s_keep_trying = false;
    xSemaphoreGive(s_policy_lock);
    esp_wifi_stop();
    s_is_connected = false;
#endif
}

void wifi_manager_keep_reconnecting(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (take_radio_operation(false) != ESP_OK) return;
#endif
    // Lift the retry limit. If the handler already ran out of retries in the
    // meantime nothing is in progress, so start a fresh attempt ourselves.
    // Under the lock, so the handler can't publish WIFI_FAIL_BIT for the old
    // attempt after we have cleared it and started a new one.
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!s_link.active || s_candidate) {
        xSemaphoreGive(s_policy_lock); xSemaphoreGive(s_connection_lock); return;
    }
#endif
    s_give_up = false;
    s_keep_trying = true;
    if (s_retries_exhausted) {
        s_retries_exhausted = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t err = esp_wifi_connect();
        if (err != ESP_OK) {
            s_link_error = err; s_retries_exhausted = true;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
#else
        esp_wifi_connect();
#endif
    }
    xSemaphoreGive(s_policy_lock);
#if CONFIG_BOARD_DRIVER_READ_PICO
    xSemaphoreGive(s_connection_lock);
#endif
}

esp_err_t wifi_manager_disconnect(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (take_radio_operation(false) != ESP_OK) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY); cancel_link_locked(); xSemaphoreGive(s_policy_lock);
    esp_err_t error = esp_wifi_disconnect();
    xSemaphoreGive(s_connection_lock);
    return error;
#else
    s_is_connected = false;
    return esp_wifi_disconnect();
#endif
}

bool wifi_manager_is_connected(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!s_driver_ready || !s_policy_lock) return false;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    bool connected = s_is_connected && s_link.ready && current_link_matches();
    if (!connected) {
        s_is_connected = s_link.ready = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    xSemaphoreGive(s_policy_lock); return connected;
#else
    return s_is_connected;
#endif
}

esp_err_t wifi_manager_get_ip(char *ip_str, size_t len)
{
    if (!ip_str || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!s_driver_ready || !s_policy_lock) return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = ESP_ERR_INVALID_STATE;
    if (s_is_connected && s_link.ready && current_link_matches()) {
        ret = esp_netif_get_ip_info(s_sta_netif, &ip_info);
        if (ret == ESP_OK && !ip_info.ip.addr) ret = ESP_ERR_INVALID_STATE;
        if (ret == ESP_OK) {
            char address[16];
            int needed = snprintf(address, sizeof(address), IPSTR, IP2STR(&ip_info.ip));
            if (needed < 0 || (size_t)needed >= sizeof(address) || (size_t)needed >= len)
                ret = ESP_ERR_INVALID_SIZE;
            else memcpy(ip_str, address, (size_t)needed + 1);
        }
    }
    xSemaphoreGive(s_policy_lock);
    return ret; // Caller output stays unchanged on stale link, IO or capacity errors.
#else
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!netif) {
        return ESP_FAIL;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        return ret;
    }

    snprintf(ip_str, len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
#endif
}

EventGroupHandle_t wifi_manager_get_event_group(void)
{
    return s_wifi_event_group;
}

esp_err_t wifi_manager_scan_checked(wifi_ap_record_t *results, int max_results, int *count)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!results || !count || max_results <= 0 || max_results > WIFI_MANAGER_SCAN_LIMIT)
        return ESP_ERR_INVALID_ARG;
    if (take_radio_operation(true) != ESP_OK) return ESP_ERR_INVALID_STATE;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    wifi_ap_record_t *temporary = NULL;
    wifi_mode_t original_mode;
    bool changed_mode = false, clear_list = false, cleanup_failed = false;
    uint16_t found = 0, fetched = 0;
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    bool active = s_link.active || !station_phase_allowed(true);
    xSemaphoreGive(s_policy_lock);
    if (active) goto done;
    err = esp_wifi_get_mode(&original_mode);
    if (err != ESP_OK) goto done;
    if (original_mode != WIFI_MODE_AP) { err = ESP_ERR_INVALID_STATE; goto done; }
    temporary = calloc((size_t)max_results, sizeof(*temporary));
    if (!temporary) { err = ESP_ERR_NO_MEM; goto done; }
    err = esp_wifi_set_mode(WIFI_MODE_APSTA);
    if (err != ESP_OK) goto done;
    changed_mode = true;
    wifi_scan_config_t scan_config = {0};
    err = esp_wifi_scan_start(&scan_config, true);
    // A rejected start did not grant ownership of somebody else's scan list.
    if (err != ESP_OK) goto done;
    clear_list = true;
    err = esp_wifi_scan_get_ap_num(&found);
    if (err != ESP_OK) goto done;
    if (found) {
        fetched = found > max_results ? (uint16_t)max_results : found;
        uint16_t capacity = fetched;
        err = esp_wifi_scan_get_ap_records(&fetched, temporary);
        if (err != ESP_OK) goto done;
        clear_list = false; // SDK releases the entire list on successful get_records.
        if (fetched > capacity) { err = ESP_ERR_INVALID_RESPONSE; goto done; }
        for (uint16_t i = 0; i < fetched; ++i) {
            if (strnlen((char *)temporary[i].ssid, sizeof(temporary[i].ssid)) > 32) {
                err = ESP_ERR_INVALID_RESPONSE; goto done;
            }
        }
    }
done:
    ;
    // Finish cleanup before publishing any caller-visible results.
    if (clear_list) {
        esp_err_t cleared = esp_wifi_clear_ap_list();
        if (cleared != ESP_OK) { cleanup_failed = true; if (err == ESP_OK) err = cleared; }
    }
    if (changed_mode) {
        esp_err_t restored = esp_wifi_set_mode(original_mode);
        if (restored != ESP_OK) { cleanup_failed = true; if (err == ESP_OK) err = restored; }
    }
    if (cleanup_failed) {
        xSemaphoreTake(s_policy_lock, portMAX_DELAY);
        cancel_link_locked();
        s_driver_ready = false; // Unknown SDK ownership: do not reuse automatically.
        xSemaphoreGive(s_policy_lock);
        ESP_LOGE(TAG, "Scan cleanup failed; WiFi manager disabled until controlled recovery");
    }
    xSemaphoreTake(s_policy_lock, portMAX_DELAY);
    if (err == ESP_OK && !station_phase_allowed(true)) err = ESP_ERR_INVALID_STATE;
    if (err == ESP_OK) {
        if (fetched) memcpy(results, temporary, fetched * sizeof(*temporary));
        *count = fetched;
    }
    xSemaphoreGive(s_policy_lock);
    free(temporary);
    xSemaphoreGive(s_connection_lock);
    return err;
#else
    (void)results; (void)max_results; (void)count;
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

int wifi_manager_scan(wifi_ap_record_t *results, int max_results)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    int count = 0;
    return wifi_manager_scan_checked(results, max_results, &count) == ESP_OK ? count : 0;
#else
    if (!results || max_results <= 0) {
        return 0;
    }

    // Save current WiFi mode
    wifi_mode_t original_mode;
    esp_err_t err = esp_wifi_get_mode(&original_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode: %s", esp_err_to_name(err));
        return 0;
    }

    // Switch to APSTA mode if currently in AP-only mode
    if (original_mode == WIFI_MODE_AP) {
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set APSTA mode: %s", esp_err_to_name(err));
            return 0;
        }
    }

    // Start blocking scan on all channels
    wifi_scan_config_t scan_config = {0};
    err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(err));
        if (original_mode == WIFI_MODE_AP) {
            esp_wifi_set_mode(original_mode);
        }
        return 0;
    }

    // Get number of APs found
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0) {
        ESP_LOGI(TAG, "No APs found");
        if (original_mode == WIFI_MODE_AP) {
            esp_wifi_set_mode(original_mode);
        }
        return 0;
    }

    // Limit to max_results
    uint16_t fetch_count = (ap_count > (uint16_t) max_results) ? (uint16_t) max_results : ap_count;
    err = esp_wifi_scan_get_ap_records(&fetch_count, results);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get scan results: %s", esp_err_to_name(err));
        if (original_mode == WIFI_MODE_AP) {
            esp_wifi_set_mode(original_mode);
        }
        return 0;
    }

    // Restore original WiFi mode
    if (original_mode == WIFI_MODE_AP) {
        esp_wifi_set_mode(original_mode);
    }

    ESP_LOGI(TAG, "WiFi scan found %d APs (returning %d)", ap_count, fetch_count);
    return (int) fetch_count;
#endif
}
