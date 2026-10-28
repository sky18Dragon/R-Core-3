#include <gtest/gtest.h>
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstring>
extern "C" {
#include "wifi_manager.h"
#include "wifi_watch.h"
#include "config_manager.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
}
namespace {
struct Registration { esp_event_base_t base; int32_t id; esp_event_handler_t handler; void *arg; void *instance; };
struct Event { esp_event_base_t base; int32_t id; std::vector<unsigned char> bytes; };
std::vector<Registration> registered;
std::deque<Event> pending;
std::vector<std::unique_ptr<bool>> locks;
EventBits_t bits;
esp_netif_t sta{}, ap_netif{};
wifi_mode_t mode;
wifi_config_t config{};
wifi_ap_record_t driver{};
bool running, sta_started, connected, fast_events, no_response, wrong_target, wrong_ip, wrong_netif;
bool no_fence, inject_old_events, empty_ip, malformed_assoc;
int connect_calls, retry_calls, config_calls, stop_calls, fence_calls;
esp_err_t mode_error, config_error, connect_error, start_error, fence_error;
esp_err_t ip_set_error, dhcp_stop_error, dhcp_start_error, storage_error;
esp_err_t dns_error, ip_read_error;
esp_err_t ps_error;
esp_err_t stop_error;
TaskHandle_t current_task;
void (*at_link_wait)();
void (*at_fence_wait)();
void (*at_scan)();
uint32_t wait_ap_owner;
int ps_calls, last_ps;
int dns_calls;
bool driver_ram, driver_nvs;
bool auth_reject;
std::string static_ip;
std::string dns_override;
std::vector<std::string> parsed_ips;
std::string last_dns_text;
wifi_network_t *mutate_at_wait;
bool change_config_at_wait;
esp_err_t scan_error, scan_num_error, scan_records_error, scan_clear_error, restore_error, get_mode_error;
uint16_t scan_found;
int scan_calls, clear_calls, record_calls;
bool scan_bad_ssid, scan_bad_count, scan_nested;
bool sta_allocated, ap_allocated, sta_attached, ap_attached, foreign_netifs, default_loop, nested_init;
esp_err_t netif_init_error, loop_error, driver_init_error, driver_deinit_error, attach_error;
esp_err_t ap_attach_error;
esp_err_t default_handler_error, hostname_error, unregister_error, clear_interface_error;
esp_err_t name_copy_error, network_copy_error;
int hostname_calls;
std::string last_hostname;
int event_allocations, mutex_allocations, allocation_fail_at, netif_calls, netif_fail_at;
int driver_init_calls, deinit_calls, destroy_calls, register_calls, register_fail_at, unregister_calls, clear_interface_calls;
void queue(esp_event_base_t base, int id, const void *p = nullptr, size_t size = 0) {
    Event e{base, id, {}};
    if (size) e.bytes.assign((const unsigned char *)p, (const unsigned char *)p + size);
    pending.push_back(std::move(e));
}
void drain() {
    int limit = 200;
    while (!pending.empty() && limit--) {
        auto e = std::move(pending.front()); pending.pop_front();
        auto copy = registered;
        for (auto r : copy)
            if (r.base == e.base && (r.id == e.id || r.id == ESP_EVENT_ANY_ID))
                r.handler(r.arg, e.base, e.id, e.bytes.empty() ? nullptr : e.bytes.data());
    }
    EXPECT_GT(limit, 0);
}
void associate() {
    connected = true; sta.up = true; sta.ip.ip.addr = empty_ip ? 0 : 0x01020304;
    driver = {}; memcpy(driver.ssid, config.sta.ssid, 32);
    if (wrong_target) memcpy(driver.ssid, "wrong", 6);
    const uint8_t mac[6] = {1,2,3,4,5,6}; memcpy(driver.bssid, mac, 6);
    wifi_event_sta_connected_t event{};
    event.ssid_len = malformed_assoc ? 33 : strnlen((char *)driver.ssid, 33);
    memcpy(event.ssid, driver.ssid, 32); memcpy(event.bssid, driver.bssid, 6);
    queue(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &event, sizeof(event));
    ip_event_got_ip_t ip{}; ip.esp_netif = wrong_netif ? &ap_netif : &sta; ip.ip_info = sta.ip;
    if (wrong_ip) ++ip.ip_info.ip.addr;
    queue(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip, sizeof(ip));
}
class ReadPicoWifiManager : public testing::Test {
protected:
    virtual bool initialize_in_setup() const { return true; }
    void SetUp() override {
        pending.clear(); registered.clear(); locks.clear(); bits = 0;
        sta = {}; ap_netif = {}; mode = WIFI_MODE_AP; config = {}; driver = {};
        running = true; sta_started = connected = false;
        fast_events = no_response = wrong_target = wrong_ip = wrong_netif = false;
        no_fence = inject_old_events = empty_ip = malformed_assoc = auth_reject = false;
        mode_error = config_error = connect_error = start_error = fence_error = ESP_OK;
        ip_set_error = dhcp_stop_error = dhcp_start_error = storage_error = ESP_OK;
        dns_error = ip_read_error = ESP_OK; dns_calls = 0; dns_override.clear();
        ps_error = ESP_OK; ps_calls = 0; last_ps = -1;
        stop_error = ESP_OK; current_task = reinterpret_cast<void *>(1); at_link_wait = nullptr;
        at_fence_wait = at_scan = nullptr;
        parsed_ips.clear(); last_dns_text.clear(); mutate_at_wait = nullptr; change_config_at_wait = false;
        driver_ram = false; driver_nvs = true;
        connect_calls = retry_calls = config_calls = stop_calls = fence_calls = 0;
        static_ip.clear();
        scan_error = scan_num_error = scan_records_error = scan_clear_error = restore_error = get_mode_error = ESP_OK;
        scan_found = 0; scan_calls = clear_calls = record_calls = 0;
        scan_bad_ssid = scan_bad_count = scan_nested = false;
        sta_allocated = ap_allocated = sta_attached = ap_attached = foreign_netifs = default_loop = nested_init = false;
        netif_init_error = loop_error = driver_init_error = driver_deinit_error = attach_error = ESP_OK;
        ap_attach_error = ESP_OK;
        default_handler_error = hostname_error = unregister_error = clear_interface_error = ESP_OK;
        name_copy_error = network_copy_error = ESP_OK; hostname_calls = 0;
        last_hostname.clear();
        event_allocations = mutex_allocations = allocation_fail_at = netif_calls = netif_fail_at = 0;
        driver_init_calls = deinit_calls = destroy_calls = register_calls = register_fail_at = unregister_calls = clear_interface_calls = 0;
        if (!initialize_in_setup()) { running = false; return; }
        ASSERT_EQ(wifi_manager_init(), ESP_OK);
        // Model an already-running AP, as in the captive portal request.
        mode = WIFI_MODE_AP; running = true; pending.clear();
    }
};
// CTest discovery runs each irreversible ownership-failure case in a fresh process.
class ReadPicoWifiInit : public ReadPicoWifiManager {
    bool initialize_in_setup() const override { return false; }
};
}
extern "C" {
esp_event_base_t WIFI_EVENT = "WIFI", IP_EVENT = "IP";
const char *esp_err_to_name(esp_err_t) { return "synthetic"; }
const char *config_manager_get_device_name(void) { return "synthetic"; }
esp_err_t config_manager_copy_device_name(char *out, size_t size) {
    if (name_copy_error) return name_copy_error;
    if (size < 10) return ESP_ERR_INVALID_SIZE;
    memcpy(out, "synthetic", 10); return ESP_OK;
}
esp_err_t config_manager_copy_network(wifi_network_t *out) {
    if (network_copy_error) return network_copy_error;
    wifi_network_t value{};
    value.static_address = !static_ip.empty();
    if (static_ip.size() >= sizeof(value.ip) || dns_override.size() >= sizeof(value.dns)) return ESP_ERR_INVALID_SIZE;
    memcpy(value.ip, static_ip.c_str(), static_ip.size() + 1);
    strcpy(value.netmask, "255.255.255.0"); strcpy(value.gateway, "192.168.1.1");
    memcpy(value.dns, dns_override.c_str(), dns_override.size() + 1);
    *out = value; return ESP_OK;
}
ip_mode_t config_manager_get_ip_mode(void) { return static_ip.empty() ? IP_MODE_DHCP : IP_MODE_STATIC; }
const char *config_manager_get_static_ip(void) { return static_ip.c_str(); }
const char *config_manager_get_static_netmask(void) { return "255.255.255.0"; }
const char *config_manager_get_static_gateway(void) { return "192.168.1.1"; }
const char *config_manager_get_dns_server(void) { return dns_override.c_str(); }
void sanitize_dhcp_hostname(const char *p, char *out, size_t size) { snprintf(out, size, "%s", p); }
EventGroupHandle_t xEventGroupCreate(void) { ++event_allocations; return allocation_fail_at == 1 ? nullptr : &bits; }
EventBits_t xEventGroupSetBits(EventGroupHandle_t, EventBits_t mask) { return bits |= mask; }
EventBits_t xEventGroupClearBits(EventGroupHandle_t, EventBits_t mask) { auto old = bits; bits &= ~mask; return old; }
EventBits_t xEventGroupWaitBits(EventGroupHandle_t, EventBits_t mask, BaseType_t clear, BaseType_t, TickType_t) {
    if (!(mask & WIFI_CONNECTED_BIT) && at_fence_wait) {
        auto callback = at_fence_wait; at_fence_wait = nullptr; callback();
    }
    if (mask & WIFI_CONNECTED_BIT) {
        if (at_link_wait) { auto callback = at_link_wait; at_link_wait = nullptr; callback(); }
        if (change_config_at_wait) { static_ip = "bad"; dns_override = "bad"; }
        if (mutate_at_wait) memset(mutate_at_wait, 0, sizeof(*mutate_at_wait));
    }
    drain(); auto out = bits; if (clear && (bits & mask)) bits &= ~mask; return out;
}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    ++mutex_allocations; if (allocation_fail_at == mutex_allocations + 1) return nullptr;
    locks.push_back(std::make_unique<bool>(false)); return locks.back().get();
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t h, TickType_t) {
    bool &locked = *static_cast<bool *>(h); if (locked) return pdFALSE;
    locked = true; return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t h) { *static_cast<bool *>(h) = false; return pdTRUE; }
void vTaskDelay(TickType_t) { drain(); }
TaskHandle_t xTaskGetCurrentTaskHandle(void) { return current_task; }
esp_err_t esp_event_loop_create_default(void) {
    if (loop_error) return loop_error;
    if (default_loop) return ESP_ERR_INVALID_STATE;
    default_loop = true; return ESP_OK;
}
esp_err_t esp_event_handler_instance_register(esp_event_base_t base, int32_t id, esp_event_handler_t fn, void *arg, esp_event_handler_instance_t *out) {
    ++register_calls;
    if (register_fail_at == register_calls) return ESP_ERR_NO_MEM;
    *out = reinterpret_cast<void *>((uintptr_t)register_calls);
    registered.push_back({base,id,fn,arg,*out}); return ESP_OK;
}
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t base, int32_t id, void *instance) {
    ++unregister_calls; if (unregister_error) return unregister_error;
    for (auto it = registered.begin(); it != registered.end(); ++it)
        if (it->base == base && it->id == id && it->instance == instance) {
            registered.erase(it); return ESP_OK;
        }
    return ESP_ERR_INVALID_ARG;
}
esp_err_t esp_event_post(esp_event_base_t base, int32_t id, const void *p, size_t n, TickType_t) {
    ++fence_calls; if (fence_error) return fence_error;
    if (!no_fence) queue(base, id, p, n); return ESP_OK;
}
esp_err_t esp_netif_init(void) { return netif_init_error; }
esp_netif_t *esp_netif_create_default_wifi_sta(void) { return &sta; }
esp_netif_t *esp_netif_create_default_wifi_ap(void) { return &ap_netif; }
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key) {
    if (foreign_netifs) return &sta;
    return !strcmp(key, "WIFI_STA_DEF") ? (sta_allocated ? &sta : nullptr) : (ap_allocated ? &ap_netif : nullptr);
}
esp_netif_t *esp_netif_new(const esp_netif_config_t *cfg) {
    ++netif_calls; if (netif_fail_at == netif_calls) return nullptr;
    if (!cfg->kind) { sta_allocated = true; return &sta; }
    ap_allocated = true; return &ap_netif;
}
void esp_netif_destroy(esp_netif_t *p) {
    ++destroy_calls;
    if (p == &sta) { EXPECT_FALSE(sta_attached); sta_allocated = false; }
    else { EXPECT_FALSE(ap_attached); ap_allocated = false; }
}
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *) { if (attach_error) return attach_error; sta_attached = true; return ESP_OK; }
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t *) { if (ap_attach_error) return ap_attach_error; ap_attached = true; return ESP_OK; }
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void) { return default_handler_error; }
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *p) {
    ++clear_interface_calls; if (clear_interface_error) return clear_interface_error;
    if (p == &sta) sta_attached = false; else ap_attached = false; return ESP_OK;
}
esp_err_t esp_netif_create_ip6_linklocal(esp_netif_t *) { return ESP_OK; }
esp_err_t esp_netif_set_hostname(esp_netif_t *, const char *name) { ++hostname_calls; last_hostname = name; return hostname_error; }
bool esp_netif_is_netif_up(esp_netif_t *p) { return p->up; }
esp_err_t esp_netif_get_ip_info(esp_netif_t *p, esp_netif_ip_info_t *out) {
    if (ip_read_error) return ip_read_error;
    *out = p->ip; return ESP_OK;
}
esp_err_t esp_netif_set_ip_info(esp_netif_t *p, const esp_netif_ip_info_t *ip) {
    if (ip_set_error) return ip_set_error;
    p->ip = *ip; return ESP_OK;
}
esp_err_t esp_netif_str_to_ip4(const char *p, esp_ip4_addr_t *out) {
    parsed_ips.emplace_back(p);
    if (!strcmp(p, "bad")) return ESP_ERR_INVALID_ARG;
    out->addr = 0x01020304; return ESP_OK;
}
esp_err_t esp_netif_set_dns_info(esp_netif_t *, int, const esp_netif_dns_info_t *) {
    last_dns_text = parsed_ips.empty() ? "" : parsed_ips.back();
    ++dns_calls; return dns_error;
}
esp_err_t esp_netif_dhcpc_start(esp_netif_t *) { return dhcp_start_error; }
esp_err_t esp_netif_dhcpc_stop(esp_netif_t *) { return dhcp_stop_error; }
esp_err_t esp_wifi_init(const wifi_init_config_t *cfg) {
    ++driver_init_calls; driver_nvs = cfg->nvs_enable != 0;
    if (nested_init) EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    return driver_init_error;
}
esp_err_t esp_wifi_deinit(void) { ++deinit_calls; return driver_deinit_error; }
esp_err_t esp_wifi_set_storage(int value) {
    if (storage_error) return storage_error;
    driver_ram = value == WIFI_STORAGE_RAM; return ESP_OK;
}
esp_err_t esp_wifi_connect(void) {
    ++connect_calls; if (connect_error) return connect_error;
    if (!running || !sta_started) return ESP_ERR_WIFI_NOT_STARTED;
    if (auth_reject) {
        wifi_event_sta_disconnected_t d{}; d.reason = WIFI_REASON_AUTH_FAIL;
        queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &d, sizeof(d));
    } else if (!no_response) associate();
    return ESP_OK;
}
esp_err_t esp_wifi_disconnect(void) {
    connected = sta.up = false;
    wifi_event_sta_disconnected_t d{}; queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &d, sizeof(d)); return ESP_OK;
}
esp_err_t esp_wifi_stop(void) {
    ++stop_calls;
    if (stop_error) return stop_error;
    if (!running) return ESP_ERR_WIFI_NOT_STARTED;
    running = sta_started = connected = sta.up = false; queue(WIFI_EVENT, WIFI_EVENT_STA_STOP); return ESP_OK;
}
esp_err_t esp_wifi_start(void) {
    if (start_error) return start_error;
    running = sta_started = true; queue(WIFI_EVENT, WIFI_EVENT_STA_START);
    if (fast_events) drain(); return ESP_OK;
}
esp_err_t esp_wifi_set_mode(wifi_mode_t next) {
    if (mode_error) return mode_error;
    if (next == WIFI_MODE_AP && restore_error) return restore_error;
    if (next == WIFI_MODE_AP && sta_started) {
        sta_started = connected = sta.up = false; queue(WIFI_EVENT, WIFI_EVENT_STA_STOP);
    }
    if (next == WIFI_MODE_APSTA && running && !sta_started) {
        sta_started = true; queue(WIFI_EVENT, WIFI_EVENT_STA_START);
    }
    mode = next; return ESP_OK;
}
esp_err_t esp_wifi_get_mode(wifi_mode_t *out) { if (get_mode_error) return get_mode_error; *out = mode; return ESP_OK; }
esp_err_t esp_wifi_set_config(int, const wifi_config_t *c) {
    ++config_calls; if (config_error) return config_error;
    EXPECT_TRUE(driver_ram); EXPECT_FALSE(driver_nvs); EXPECT_FALSE(connected); config = *c; return ESP_OK;
}
esp_err_t esp_wifi_set_ps(int value) { ++ps_calls; last_ps = value; return ps_error; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *out) { if (!connected) return ESP_FAIL; *out = driver; return ESP_OK; }
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *, bool blocking) {
    EXPECT_TRUE(blocking); ++scan_calls;
    if (at_scan) { auto callback = at_scan; at_scan = nullptr; callback(); }
    if (scan_nested) {
        wifi_ap_record_t record{}; int count = 77;
        EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(count, 77);
        EXPECT_EQ(wifi_manager_test_credentials("network", "password"), ESP_ERR_INVALID_STATE);
    }
    return scan_error;
}
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *out) { *out = scan_found; return scan_num_error; }
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *count, wifi_ap_record_t *out) {
    ++record_calls;
    for (int i = 0; i < *count; ++i) {
        out[i] = {}; memcpy(out[i].ssid, "network", 8);
        if (scan_bad_ssid) memset(out[i].ssid, 'x', sizeof(out[i].ssid));
    }
    if (scan_bad_count) ++*count;
    return scan_records_error;
}
esp_err_t esp_wifi_clear_ap_list(void) { ++clear_calls; return scan_clear_error; }
}

TEST_F(ReadPicoWifiManager, ScanPublishesOnlyAfterApRestored) {
    scan_found = 25; std::array<wifi_ap_record_t, 20> records{}; int count = -1;
    EXPECT_EQ(wifi_manager_scan_checked(records.data(), 20, &count), ESP_OK);
    EXPECT_EQ(count, 20); EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_EQ(clear_calls, 0);
    EXPECT_STREQ((char *)records[19].ssid, "network"); EXPECT_EQ(connect_calls, 0);
}

TEST_F(ReadPicoWifiManager, ApBeginCancelsOldStationAndCallbacksCannotRearmIt) {
    no_response = true;
    ASSERT_EQ(wifi_manager_connect("old", "password"), ESP_ERR_TIMEOUT);
    uint32_t owner = 0; esp_netif_t *netif = nullptr;
    ASSERT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_OK);
    EXPECT_NE(owner, 0u); EXPECT_EQ(netif, &ap_netif);
    auto before = connect_calls;
    wifi_event_sta_disconnected_t disconnected{};
    queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnected, sizeof(disconnected));
    queue(WIFI_EVENT, WIFI_EVENT_STA_START); associate(); drain();
    wifi_manager_keep_reconnecting();
    EXPECT_EQ(connect_calls, before); EXPECT_FALSE(wifi_manager_is_connected());
    ASSERT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_READY), ESP_OK);
    wifi_manager_keep_reconnecting(); EXPECT_EQ(connect_calls, before);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(owner), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ApStartupBlocksAllMutatorsWithoutGivingReservedMutex) {
    uint32_t owner = 0; esp_netif_t *netif = nullptr;
    ASSERT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_OK);
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_netif(), nullptr);
    auto stops = stop_calls; auto configs = config_calls; auto power = ps_calls;
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_test_credentials("candidate", "password"), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_disconnect(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_apply_ip_config(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_update_hostname(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
    wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(count, 77); EXPECT_EQ(stop_calls, stops); EXPECT_EQ(config_calls, configs); EXPECT_EQ(ps_calls, power);
    uint32_t other = 99; auto other_netif = &sta;
    EXPECT_EQ(wifi_manager_provisioning_begin(&other, &other_netif), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(other, 99u); EXPECT_EQ(other_netif, &sta);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_READY), ESP_OK);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ActiveApAllowsCandidateScanAndPowerButNotStationTakeover) {
    uint32_t owner; esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_READY), ESP_OK);
    EXPECT_EQ(wifi_manager_init(), ESP_OK); // No replacement or reset of the live AP owner.
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_disconnect(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_apply_ip_config(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_update_hostname(), ESP_OK);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_OK);
    wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_OK); EXPECT_EQ(count, 0);
    EXPECT_EQ(wifi_manager_test_credentials("candidate", "password"), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(owner), ESP_OK);
    EXPECT_FALSE(wifi_manager_is_connected());
    EXPECT_EQ(wifi_manager_set_performance_mode(false), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_STOPPED), ESP_OK);
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ForeignTaskCannotFinishOrQuiesceAnApCallReservation) {
    uint32_t owner; esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_OK);
    current_task = reinterpret_cast<void *>(2);
    EXPECT_EQ(wifi_manager_provisioning_quiesce(owner), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_FAILED), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
    current_task = reinterpret_cast<void *>(1);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_FAILED), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_netif(), nullptr);
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    current_task = reinterpret_cast<void *>(2);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(owner), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, InvalidOwnerAndOutcomeCannotReleaseApReservation) {
    uint32_t owner; esp_netif_t *netif;
    EXPECT_EQ(wifi_manager_provisioning_begin(nullptr, &netif), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_provisioning_begin(&owner, nullptr), ESP_ERR_INVALID_ARG);
    ASSERT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_quiesce(owner + 1), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner + 1, WIFI_PROVISION_FAILED), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, static_cast<wifi_provisioning_outcome_t>(99)), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_finish(owner, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, RetainedFaultOwnerBlocksReuseUntilExplicitExitAndNeverReusesToken) {
    uint32_t first; esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&first, &netif), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(first, WIFI_PROVISION_FAILED), ESP_OK);
    uint32_t second = 99;
    EXPECT_EQ(wifi_manager_provisioning_begin(&second, &netif), ESP_ERR_INVALID_STATE); EXPECT_EQ(second, 99u);
    EXPECT_EQ(wifi_manager_test_credentials("candidate", "password"), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(first), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(first, WIFI_PROVISION_STOPPED), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_begin(&second, &netif), ESP_OK); EXPECT_NE(second, first);
    EXPECT_EQ(wifi_manager_provisioning_finish(first, WIFI_PROVISION_FAILED), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_provisioning_finish(second, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, BusyCandidateIsCanceledBeforeApExitAndItsMutexIsNotGivenByFaultFinish) {
    esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&wait_ap_owner, &netif), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_READY), ESP_OK);
    at_link_wait = [] {
        EXPECT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_FAILED), ESP_OK);
        // A fault finish may not give the in-flight candidate's mutex.
        EXPECT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_ERR_INVALID_STATE);
    };
    EXPECT_EQ(wifi_manager_test_credentials("candidate", "password"), ESP_FAIL);
    EXPECT_FALSE(wifi_manager_is_connected());
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_STOPPED), ESP_OK);
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ConnectionWaitRejectsNestedMutatorsAndApBeginWithoutSdkChanges) {
    at_link_wait = [] {
        auto stops = stop_calls; auto power = ps_calls; auto configs = config_calls;
        uint32_t owner = 99; auto netif = &sta;
        EXPECT_EQ(wifi_manager_provisioning_begin(&owner, &netif), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(owner, 99u); EXPECT_EQ(netif, &sta);
        EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_disconnect(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_apply_ip_config(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_update_hostname(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
        wifi_manager_keep_reconnecting();
        EXPECT_EQ(stop_calls, stops); EXPECT_EQ(ps_calls, power); EXPECT_EQ(config_calls, configs);
    };
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_OK);
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ApCancelDuringFenceCannotArmCandidateAfterCancellation) {
    esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&wait_ap_owner, &netif), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_READY), ESP_OK);
    at_fence_wait = [] {
        EXPECT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_FAILED), ESP_OK);
    };
    uint32_t candidate = 99; wifi_network_t network{};
    EXPECT_EQ(wifi_manager_test_network_owned("candidate", "password", &network, &candidate), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(candidate, 99u); EXPECT_EQ(connect_calls, 0); EXPECT_FALSE(wifi_manager_is_connected());
    ASSERT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ApCancelDuringScanRetainsCallerOutputAndFinishesOwnedListCleanup) {
    esp_netif_t *netif;
    ASSERT_EQ(wifi_manager_provisioning_begin(&wait_ap_owner, &netif), ESP_OK);
    ASSERT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_READY), ESP_OK);
    at_scan = [] {
        EXPECT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_FAILED), ESP_OK);
        EXPECT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_ERR_INVALID_STATE);
    };
    scan_found = 1; wifi_ap_record_t record{}; strcpy((char *)record.ssid, "unchanged"); int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(count, 77); EXPECT_STREQ((char *)record.ssid, "unchanged");
    EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_EQ(record_calls, 1); EXPECT_EQ(clear_calls, 0);
    ASSERT_EQ(wifi_manager_provisioning_quiesce(wait_ap_owner), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_finish(wait_ap_owner, WIFI_PROVISION_STOPPED), ESP_OK);
}
TEST_F(ReadPicoWifiManager, CheckedStationStopFailureIsReportedAndDisablesAutomaticReuse) {
    ASSERT_EQ(wifi_manager_connect("station", "password"), ESP_OK);
    stop_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_FAIL);
    EXPECT_FALSE(wifi_manager_is_connected());
    auto calls = stop_calls; stop_error = ESP_OK;
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_ERR_INVALID_STATE); EXPECT_EQ(stop_calls, calls);
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_connect("station", "password"), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoWifiManager, AlreadyStoppedStationIsConfirmedAndLateCallbacksRemainCanceled) {
    no_response = true; ASSERT_EQ(wifi_manager_connect("station", "password"), ESP_ERR_TIMEOUT);
    stop_error = ESP_ERR_WIFI_NOT_STARTED;
    EXPECT_EQ(wifi_manager_stop_connecting_checked(), ESP_OK);
    auto connects = connect_calls;
    queue(WIFI_EVENT, WIFI_EVENT_STA_START); associate(); drain(); wifi_manager_keep_reconnecting();
    EXPECT_EQ(connect_calls, connects); EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, EmptyScanClearsOwnedListAndRestoresAp) {
    wifi_ap_record_t record{}; memset(&record, 0x5a, sizeof(record)); auto before = record; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_OK);
    EXPECT_EQ(count, 0); EXPECT_EQ(memcmp(&record, &before, sizeof(record)), 0);
    EXPECT_EQ(clear_calls, 1); EXPECT_EQ(record_calls, 0); EXPECT_EQ(mode, WIFI_MODE_AP);
}
TEST_F(ReadPicoWifiManager, InvalidScanArgumentsDoNotTouchSdkOrOutputs) {
    wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(nullptr, 1, &count), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_scan_checked(&record, 0, &count), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_scan_checked(&record, 21, &count), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(count, 77); EXPECT_EQ(scan_calls, 0);
}
TEST_F(ReadPicoWifiManager, ScanRejectsActiveCandidateAndNonApMode) {
    ASSERT_EQ(wifi_manager_test_credentials("network", "password"), ESP_OK);
    wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(count, 77); EXPECT_EQ(scan_calls, 0);
}
TEST_F(ReadPicoWifiManager, ScanRejectsStationModeWithoutSwitching) {
    mode = WIFI_MODE_STA; wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mode, WIFI_MODE_STA); EXPECT_EQ(scan_calls, 0);
}
TEST_F(ReadPicoWifiManager, ScanAndCandidateCannotInterleave) {
    scan_nested = true; wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_OK);
    EXPECT_EQ(scan_calls, 1); EXPECT_EQ(connect_calls, 0);
    EXPECT_EQ(wifi_manager_test_credentials("network", "password"), ESP_OK);
}
TEST_F(ReadPicoWifiManager, ScanGetModeOrSwitchFailureDoesNotStartScan) {
    wifi_ap_record_t record{}; int count = 77; get_mode_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    get_mode_error = ESP_OK; mode_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    EXPECT_EQ(count, 77); EXPECT_EQ(scan_calls, 0); EXPECT_EQ(clear_calls, 0);
}
TEST_F(ReadPicoWifiManager, ScanStartFailureDoesNotClearUnownedList) {
    scan_error = ESP_FAIL; wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    EXPECT_EQ(count, 77); EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_EQ(clear_calls, 0);
}
TEST_F(ReadPicoWifiManager, ScanCountAndRecordsErrorsClearListAndPreserveOutput) {
    wifi_ap_record_t record{}; memset(&record, 0x5a, sizeof(record)); auto before = record; int count = 77;
    scan_num_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    scan_num_error = ESP_OK; scan_found = 1; scan_records_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    EXPECT_EQ(count, 77); EXPECT_EQ(memcmp(&record, &before, sizeof(record)), 0);
    EXPECT_EQ(clear_calls, 2); EXPECT_EQ(mode, WIFI_MODE_AP);
}
TEST_F(ReadPicoWifiManager, MalformedScanRecordsAreNotPublished) {
    wifi_ap_record_t record{}; int count = 77; scan_found = 1; scan_bad_ssid = true;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_RESPONSE);
    scan_bad_ssid = false; scan_bad_count = true;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_RESPONSE);
    EXPECT_EQ(count, 77); EXPECT_EQ(mode, WIFI_MODE_AP);
}
TEST_F(ReadPicoWifiManager, ScanRestoreFailureDisablesReuseAndPreservesOutput) {
    scan_found = 1; restore_error = ESP_FAIL;
    wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    EXPECT_EQ(count, 77); EXPECT_EQ(record.ssid[0], 0);
    restore_error = ESP_OK;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_test_credentials("network", "password"), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoWifiManager, ScanClearFailureIsNotEmptySuccess) {
    scan_clear_error = ESP_FAIL; wifi_ap_record_t record{}; int count = 77;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    EXPECT_EQ(count, 77); EXPECT_EQ(mode, WIFI_MODE_AP);
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_ERR_INVALID_STATE);
}

TEST_F(ReadPicoWifiManager, CandidateKeepsApAndConnectsOnceAfterCheckedConfig) {
    ASSERT_EQ(wifi_manager_test_credentials("network", "password"), ESP_OK);
    EXPECT_EQ(mode, WIFI_MODE_APSTA); EXPECT_TRUE(running); EXPECT_TRUE(wifi_manager_is_connected());
    EXPECT_EQ(connect_calls, 1); EXPECT_EQ(fence_calls, 1); EXPECT_EQ(stop_calls, 0);
}
TEST_F(ReadPicoWifiManager, FastNormalSuccessBeforeStartReturnsIsNotErased) {
    fast_events = true; ASSERT_EQ(wifi_manager_connect("network", "password"), ESP_OK);
    EXPECT_TRUE(wifi_manager_is_connected()); EXPECT_EQ(connect_calls, 1);
}
TEST_F(ReadPicoWifiManager, OldConnectedBitAndIpEventCannotSatisfyNewCandidate) {
    bits = WIFI_CONNECTED_BIT; no_response = true;
    wifi_event_sta_connected_t old{}; memcpy(old.ssid, "old", 3); old.ssid_len = 3;
    queue(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &old, sizeof(old));
    ip_event_got_ip_t ip{}; ip.esp_netif = &sta; ip.ip_info.ip.addr = 99;
    queue(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip, sizeof(ip));
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_TIMEOUT);
    EXPECT_FALSE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_TRUE(running);
}
TEST_F(ReadPicoWifiManager, WrongTargetWrongIpWrongInterfaceAndMalformedAssociationNeverPass) {
    for (int which = 0; which < 5; ++which) {
        wrong_target = which == 0; wrong_ip = which == 1; wrong_netif = which == 2;
        empty_ip = which == 3; malformed_assoc = which == 4;
        EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_TIMEOUT);
        EXPECT_FALSE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_AP);
    }
}
TEST_F(ReadPicoWifiManager, SdkErrorsAndMissingFenceReturnErrorsWithoutConnectionOrSave) {
    config_error = ESP_FAIL; EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
    EXPECT_EQ(connect_calls, 0); config_error = 0;
    fence_error = ESP_ERR_NO_MEM; EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_NO_MEM);
    fence_error = 0; no_fence = true;
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_TIMEOUT); no_fence = false;
    mode_error = ESP_FAIL; EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
    mode_error = 0; connect_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
    EXPECT_FALSE(wifi_manager_is_connected());
    // NO NVS save/erase symbols are provided to this real-manager fixture.
}
TEST_F(ReadPicoWifiManager, RejectedPasswordHasBoundedRetriesAndApIsRestored) {
    auth_reject = true; EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
    EXPECT_EQ(connect_calls, 6); EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, StopAndLostIpInvalidateCachedReadiness) {
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    queue(IP_EVENT, IP_EVENT_STA_LOST_IP); drain(); EXPECT_FALSE(wifi_manager_is_connected());
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    wifi_manager_stop_connecting(); EXPECT_FALSE(wifi_manager_is_connected());
    auto before = connect_calls; drain(); EXPECT_EQ(connect_calls, before);
}
TEST_F(ReadPicoWifiManager, LiveDriverLossCannotBeHiddenByReadyBitAndNewAttemptStillWorks) {
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    connected = false; EXPECT_FALSE(wifi_manager_is_connected());
    ASSERT_EQ(wifi_manager_test_credentials("other", "password"), ESP_OK);
}
TEST_F(ReadPicoWifiManager, FullWireSsidHexPskAndOpenNetworkAreNotTruncated) {
    auto ssid = std::string(32, 's'), psk = std::string(64, 'f');
    ASSERT_EQ(wifi_manager_test_credentials(ssid.c_str(), psk.c_str()), ESP_OK);
    EXPECT_EQ(memcmp(config.sta.ssid, ssid.data(), 32), 0); EXPECT_EQ(memcmp(config.sta.password, psk.data(), 64), 0);
    ASSERT_EQ(wifi_manager_test_credentials("open", ""), ESP_OK);
    EXPECT_EQ(config.sta.threshold.authmode, WIFI_AUTH_OPEN);
}
TEST_F(ReadPicoWifiManager, InvalidInputsAndInvalidIpConfigNeverStartCandidate) {
    EXPECT_EQ(wifi_manager_test_credentials(nullptr, "password"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_test_credentials("new", "short"), ESP_ERR_INVALID_ARG); EXPECT_EQ(config_calls, 0);
    static_ip = "bad"; EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(connect_calls, 0); EXPECT_EQ(mode, WIFI_MODE_AP);
}
TEST_F(ReadPicoWifiManager, DhcpAndIpErrorsCannotUseOldAddressOrClaimSuccess) {
    for (int which = 0; which < 3; ++which) {
        ip_set_error = which == 0 ? ESP_FAIL : ESP_OK;
        dhcp_stop_error = which == 1 ? ESP_FAIL : ESP_OK;
        dhcp_start_error = which == 2 ? ESP_FAIL : ESP_OK;
        sta.ip.ip.addr = 99;
        EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
        EXPECT_EQ(connect_calls, 0); EXPECT_FALSE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_AP);
    }
}
TEST_F(ReadPicoWifiManager, AlreadyStoppedDhcpStillClearsRetainedIpAndOrdinaryTimeoutStaysPending) {
    dhcp_stop_error = ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED;
    sta.ip.ip.addr = 99; no_response = true;
    EXPECT_EQ(wifi_manager_connect("new", "password"), ESP_ERR_TIMEOUT);
    EXPECT_EQ(sta.ip.ip.addr, 0u); EXPECT_FALSE(wifi_manager_is_connected());
    no_response = false; wifi_manager_keep_reconnecting();
    wifi_manager_stop_connecting(); EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, DriverPersistenceCannotBeImplicitlyEnabledForCandidate) {
    EXPECT_TRUE(driver_ram); EXPECT_FALSE(driver_nvs);
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    EXPECT_TRUE(driver_ram); EXPECT_FALSE(driver_nvs);
}
TEST_F(ReadPicoWifiInit, FailedRamStorageSetupCannotFallBackToFlashCandidate) {
    storage_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(config_calls, 0); EXPECT_FALSE(wifi_manager_is_connected());
    EXPECT_EQ(deinit_calls, 1); EXPECT_EQ(netif_calls, 0);
}

TEST_F(ReadPicoWifiInit, CallsBeforeInitializationDoNotTouchRadioOrNullMutex) {
    wifi_manager_stop_connecting(); wifi_manager_keep_reconnecting();
    EXPECT_EQ(wifi_manager_disconnect(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(stop_calls, 0); EXPECT_EQ(connect_calls, 0); EXPECT_EQ(ps_calls, 0);
}
TEST_F(ReadPicoWifiInit, ProvisioningInterfaceIsPublishedOnlyWhileCheckedManagerIsReady) {
    EXPECT_EQ(wifi_manager_provisioning_netif(), nullptr);
    ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_netif(), &ap_netif);
    ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_netif(), &ap_netif);
    // The existing scan-cleanup fault closes manager readiness, without destroying netif.
    mode = WIFI_MODE_AP; running = true; restore_error = ESP_FAIL;
    wifi_ap_record_t records[WIFI_MANAGER_SCAN_LIMIT]; int count = 0;
    EXPECT_NE(wifi_manager_scan_checked(records, WIFI_MANAGER_SCAN_LIMIT, &count), ESP_OK);
    EXPECT_EQ(wifi_manager_provisioning_netif(), nullptr);
}
TEST_F(ReadPicoWifiInit, RepeatedSuccessPreservesResourcesAndVerifiedConnection) {
    ASSERT_EQ(wifi_manager_init(), ESP_OK);
    mode = WIFI_MODE_AP; running = true;
    ASSERT_EQ(wifi_manager_test_credentials("network", "password"), ESP_OK);
    auto handles = wifi_manager_get_event_group();
    ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(wifi_manager_get_event_group(), handles); EXPECT_TRUE(wifi_manager_is_connected());
    EXPECT_EQ(mode, WIFI_MODE_APSTA); EXPECT_EQ(driver_init_calls, 1);
    EXPECT_EQ(event_allocations, 1); EXPECT_EQ(mutex_allocations, 2); EXPECT_EQ(register_calls, 5);
    EXPECT_EQ(netif_calls, 2); EXPECT_EQ(deinit_calls, 0);
}
TEST_F(ReadPicoWifiInit, ExistingDefaultLoopIsBorrowedWithoutRecreationOrDeletion) {
    default_loop = true; ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_TRUE(default_loop); EXPECT_EQ(registered.size(), 5u); EXPECT_FALSE(running);
}
TEST_F(ReadPicoWifiInit, ForeignDefaultInterfacesAreNotAdoptedOrDestroyed) {
    foreign_netifs = true; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(driver_init_calls, 0); EXPECT_EQ(netif_calls, 0); EXPECT_EQ(destroy_calls, 0);
    foreign_netifs = false; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
class WifiInitAllocation : public ReadPicoWifiInit, public testing::WithParamInterface<int> {};
TEST_P(WifiInitAllocation, PartialSynchronizationAllocationRetryReusesExistingHandles) {
    allocation_fail_at = GetParam();
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(driver_init_calls, 0); EXPECT_EQ(locks.size(), GetParam() == 1 ? 2u : 1u);
    allocation_fail_at = 0; ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(locks.size(), 2u); EXPECT_EQ(driver_init_calls, 1);
    EXPECT_EQ(event_allocations, GetParam() == 1 ? 2 : 1);
    EXPECT_EQ(mutex_allocations, GetParam() == 1 ? 2 : 3);
}
INSTANTIATE_TEST_SUITE_P(EachAllocation, WifiInitAllocation, testing::Values(1, 2, 3));
TEST_F(ReadPicoWifiInit, NetifCoreErrorDoesNotInitializeDriverAndCanExplicitlyRetry) {
    netif_init_error = ESP_FAIL; EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_EQ(driver_init_calls, 0); EXPECT_FALSE(default_loop);
    netif_init_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(mutex_allocations, 2); EXPECT_EQ(event_allocations, 1);
}
TEST_F(ReadPicoWifiInit, EventLoopErrorDoesNotInitializeDriverAndCanExplicitlyRetry) {
    loop_error = ESP_ERR_NO_MEM; EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(driver_init_calls, 0); EXPECT_FALSE(default_loop);
    loop_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
TEST_F(ReadPicoWifiInit, DriverInitFailureCannotDeinitForeignOrUnprovenDriver) {
    driver_init_error = ESP_ERR_INVALID_STATE; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(deinit_calls, 0); EXPECT_EQ(netif_calls, 0); EXPECT_FALSE(driver_nvs);
    driver_init_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(driver_init_calls, 1);
}
TEST_F(ReadPicoWifiInit, CleanRamFailureReleasesOwnedDriverAndAllowsExplicitRetry) {
    storage_error = ESP_ERR_NO_MEM; EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(deinit_calls, 1); EXPECT_FALSE(wifi_manager_is_connected());
    storage_error = ESP_OK; ASSERT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(driver_init_calls, 2); EXPECT_EQ(event_allocations, 1); EXPECT_EQ(mutex_allocations, 2);
}
class WifiInitNetifAllocation : public ReadPicoWifiInit, public testing::WithParamInterface<int> {};
TEST_P(WifiInitNetifAllocation, NetifAllocationFailureReleasesOnlyOwnedObjects) {
    netif_fail_at = GetParam(); EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(deinit_calls, 1); EXPECT_EQ(destroy_calls, GetParam() - 1);
    EXPECT_FALSE(sta_allocated); EXPECT_FALSE(ap_allocated); EXPECT_TRUE(default_loop);
    netif_fail_at = 0; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
INSTANTIATE_TEST_SUITE_P(EachInterface, WifiInitNetifAllocation, testing::Values(1, 2));
TEST_F(ReadPicoWifiInit, FailedStationAttachRetainsUnknownOwnershipAndBlocksRetry) {
    attach_error = ESP_FAIL; EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_TRUE(sta_allocated); EXPECT_EQ(deinit_calls, 0); EXPECT_EQ(destroy_calls, 0);
    attach_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(driver_init_calls, 1); EXPECT_EQ(netif_calls, 1);
}
TEST_F(ReadPicoWifiInit, FailedApAttachRetainsBothInterfacesAndBlocksRetry) {
    ap_attach_error = ESP_FAIL; EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_TRUE(sta_attached); EXPECT_TRUE(ap_allocated); EXPECT_EQ(deinit_calls, 0);
    ap_attach_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(netif_calls, 2); EXPECT_EQ(destroy_calls, 0);
}
TEST_F(ReadPicoWifiInit, DefaultHandlersFailureReleasesBothNetifsBeforeExplicitRetry) {
    default_handler_error = ESP_ERR_NO_MEM; EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(destroy_calls, 2); EXPECT_EQ(deinit_calls, 1); EXPECT_TRUE(registered.empty());
    default_handler_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
TEST_F(ReadPicoWifiInit, HostnameFailureIsNotIgnoredAndReleasesInterfaces) {
    hostname_error = ESP_FAIL; EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_EQ(destroy_calls, 2); EXPECT_EQ(deinit_calls, 1); EXPECT_EQ(register_calls, 0);
    hostname_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
class WifiInitRegistration : public ReadPicoWifiInit, public testing::WithParamInterface<int> {};
TEST_P(WifiInitRegistration, RegistrationFailureUnregistersExactlySuccessfulInstances) {
    register_fail_at = GetParam(); EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(unregister_calls, GetParam() - 1); EXPECT_TRUE(registered.empty());
    EXPECT_EQ(deinit_calls, 1); EXPECT_EQ(destroy_calls, 2);
    register_fail_at = 0; EXPECT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(registered.size(), 5u); EXPECT_EQ(event_allocations, 1); EXPECT_EQ(mutex_allocations, 2);
}
INSTANTIATE_TEST_SUITE_P(EachHandler, WifiInitRegistration, testing::Values(1, 2, 3, 4, 5));
TEST_F(ReadPicoWifiInit, InitialModeFailureUnregistersAllBeforeReleasingDriverAndNetifs) {
    mode_error = ESP_FAIL; EXPECT_EQ(wifi_manager_init(), ESP_FAIL);
    EXPECT_EQ(unregister_calls, 5); EXPECT_TRUE(registered.empty());
    EXPECT_EQ(deinit_calls, 1); EXPECT_EQ(destroy_calls, 2);
    mode_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_OK);
}
TEST_F(ReadPicoWifiInit, UnregisterFailurePreservesPrimaryErrorAndDoesNotReleaseCallbackResources) {
    mode_error = ESP_ERR_NO_MEM; unregister_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(deinit_calls, 0); EXPECT_EQ(destroy_calls, 0); EXPECT_EQ(registered.size(), 5u);
    queue(WIFI_EVENT, WIFI_EVENT_STA_START); drain(); EXPECT_EQ(connect_calls, 0);
    mode_error = unregister_error = ESP_OK;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE); EXPECT_EQ(driver_init_calls, 1);
}
TEST_F(ReadPicoWifiInit, DriverCleanupFailureIsLatchedWithoutRetryOrSuccessPublication) {
    storage_error = ESP_ERR_NO_MEM; driver_deinit_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM); EXPECT_EQ(deinit_calls, 1);
    storage_error = driver_deinit_error = ESP_OK;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE); EXPECT_EQ(deinit_calls, 1);
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoWifiInit, InterfaceCleanupFailureDoesNotDestroyOrReuseUncertainHandle) {
    hostname_error = ESP_ERR_NO_MEM; clear_interface_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_NO_MEM); EXPECT_EQ(deinit_calls, 1);
    EXPECT_EQ(clear_interface_calls, 1); EXPECT_EQ(destroy_calls, 0);
    hostname_error = clear_interface_error = ESP_OK;
    EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE); EXPECT_EQ(clear_interface_calls, 1);
}
TEST_F(ReadPicoWifiInit, ReentrantInitializationIsRejectedWithoutDuplicateAllocation) {
    nested_init = true; EXPECT_EQ(wifi_manager_init(), ESP_OK);
    EXPECT_EQ(driver_init_calls, 1); EXPECT_EQ(event_allocations, 1); EXPECT_EQ(mutex_allocations, 2);
}
TEST_F(ReadPicoWifiManager, ReinitializationCannotClearRuntimeScanCleanupLatch) {
    scan_clear_error = ESP_FAIL; wifi_ap_record_t record{}; int count = -1;
    EXPECT_EQ(wifi_manager_scan_checked(&record, 1, &count), ESP_FAIL);
    scan_clear_error = ESP_OK; EXPECT_EQ(wifi_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(driver_init_calls, 1); EXPECT_EQ(netif_calls, 2);
}

TEST_F(ReadPicoWifiManager, InvalidDnsIsRejectedBeforeChangingPreviouslyVerifiedLink) {
    ASSERT_EQ(wifi_manager_test_credentials("good", "password"), ESP_OK);
    dns_override = "bad";
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_INVALID_ARG);
    EXPECT_TRUE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_APSTA);
    EXPECT_EQ(config_calls, 1); EXPECT_EQ(dns_calls, 0);
}
TEST_F(ReadPicoWifiManager, BusyNetworkSnapshotDoesNotChangeVerifiedLinkOrCallSdk) {
    ASSERT_EQ(wifi_manager_test_credentials("good", "password"), ESP_OK);
    int before_stop = stop_calls, before_config = config_calls, before_dns = dns_calls;
    network_copy_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(wifi_manager_is_connected());
    EXPECT_EQ(stop_calls, before_stop); EXPECT_EQ(config_calls, before_config); EXPECT_EQ(dns_calls, before_dns);
}
TEST_F(ReadPicoWifiManager, BusyNameSnapshotDoesNotSetHostnameAndReleasesRadioControl) {
    int before = hostname_calls;
    name_copy_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(wifi_manager_update_hostname(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(hostname_calls, before);
    name_copy_error = ESP_OK;
    EXPECT_EQ(wifi_manager_update_hostname(), ESP_OK); EXPECT_EQ(hostname_calls, before + 1);
}
TEST_F(ReadPicoWifiManager, NamedHostnameUsesProvidedValueWithoutReadingBusyConfig) {
    name_copy_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(wifi_manager_update_hostname_named("explicit"), ESP_OK); EXPECT_EQ(last_hostname, "explicit");
}
TEST_F(ReadPicoWifiManager, InvalidNamedHostnameHasNoSdkEffect) {
    int before = hostname_calls;
    EXPECT_EQ(wifi_manager_update_hostname_named(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_update_hostname_named(""), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_update_hostname_named("bad\nname"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_update_hostname_named(std::string(64, 'x').c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(hostname_calls, before);
}
TEST_F(ReadPicoWifiManager, NamedHostnameSdkFailureReleasesRadioForExplicitRetry) {
    hostname_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_update_hostname_named("candidate"), ESP_FAIL);
    hostname_error = ESP_OK;
    EXPECT_EQ(wifi_manager_update_hostname_named("candidate"), ESP_OK); EXPECT_EQ(last_hostname, "candidate");
}
TEST_F(ReadPicoWifiManager, DnsSdkFailureIsPropagatedAndFreshRetryCanSucceed) {
    dns_override = "192.168.1.1"; dns_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_ERR_NO_MEM);
    EXPECT_FALSE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_AP);
    dns_error = ESP_OK;
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    EXPECT_TRUE(wifi_manager_is_connected()); EXPECT_EQ(dns_calls, 2);
}
TEST_F(ReadPicoWifiManager, StaticGatewayDnsMustBeAppliedButDhcpWithoutOverrideSkipsSetter) {
    dns_error = ESP_FAIL;
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    EXPECT_EQ(dns_calls, 0);
    static_ip = "192.168.1.42";
    EXPECT_EQ(wifi_manager_test_credentials("new", "password"), ESP_FAIL);
    EXPECT_EQ(dns_calls, 1); EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, IpOutputIsCompleteOrUnchangedOnCapacityAndDriverErrors) {
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    char out[20] = "unchanged";
    EXPECT_EQ(wifi_manager_get_ip(out, 3), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(out, "unchanged");
    EXPECT_EQ(wifi_manager_get_ip(out, sizeof(out)), ESP_OK);
    EXPECT_STREQ(out, "4.3.2.1"); strcpy(out, "unchanged");
    ip_read_error = ESP_FAIL;
    EXPECT_NE(wifi_manager_get_ip(out, sizeof(out)), ESP_OK); EXPECT_STREQ(out, "unchanged");
    EXPECT_EQ(wifi_manager_get_ip(nullptr, 3), ESP_ERR_INVALID_ARG);
}
TEST_F(ReadPicoWifiManager, IpOutputRejectsLostOrCanceledLinkDespiteRetainedAddress) {
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    char out[20] = "unchanged";
    queue(IP_EVENT, IP_EVENT_STA_LOST_IP); drain();
    EXPECT_NE(sta.ip.ip.addr, 0u);
    EXPECT_EQ(wifi_manager_get_ip(out, sizeof(out)), ESP_ERR_INVALID_STATE); EXPECT_STREQ(out, "unchanged");
    ASSERT_EQ(wifi_manager_test_credentials("new", "password"), ESP_OK);
    wifi_manager_disconnect();
    EXPECT_EQ(wifi_manager_get_ip(out, sizeof(out)), ESP_ERR_INVALID_STATE); EXPECT_STREQ(out, "unchanged");
}
TEST_F(ReadPicoWifiManager, StartCallbackAndBackgroundRetrySdkFailurePublishFailNotReadiness) {
    connect_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(wifi_manager_connect("new", "password"), ESP_ERR_NO_MEM);
    EXPECT_FALSE(wifi_manager_is_connected());
    connect_error = ESP_OK; no_response = true;
    EXPECT_EQ(wifi_manager_connect("new", "password"), ESP_ERR_TIMEOUT);
    wifi_event_sta_disconnected_t disc{};
    for (int i = 0; i < 6; ++i)
        queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disc, sizeof(disc));
    drain();
    EXPECT_NE(bits & WIFI_FAIL_BIT, 0u);
    connect_error = ESP_ERR_NO_MEM; auto before = connect_calls;
    wifi_manager_keep_reconnecting();
    EXPECT_EQ(connect_calls, before + 1); EXPECT_NE(bits & WIFI_FAIL_BIT, 0u);
    EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, SupervisorRestartsReconnectAfterOutageOutlastsQuickRetries) {
    ASSERT_EQ(wifi_manager_connect("new", "password"), ESP_OK);
    wifi_manager_supervise(0);  // connected: resets any earlier backoff
    auto stops = stop_calls;
    no_response = true;         // router reboots and stays away
    wifi_event_sta_disconnected_t disc{}; disc.reason = 200;  // BEACON_TIMEOUT: AP gone, not an auth rejection
    for (int i = 0; i < 6; ++i)
        queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disc, sizeof(disc));
    drain();
    ASSERT_NE(bits & WIFI_FAIL_BIT, 0u);  // quick retries exhausted, nothing in progress
    auto before = connect_calls;
    wifi_manager_supervise(1000);         // schedules
    wifi_manager_supervise(1000 + WIFI_WATCH_FIRST_RETRY_MS - 1);
    EXPECT_EQ(connect_calls, before);
    wifi_manager_supervise(1000 + WIFI_WATCH_FIRST_RETRY_MS);
    EXPECT_EQ(connect_calls, before + 1);
    EXPECT_EQ(bits & WIFI_FAIL_BIT, 0u);
    // The restarted attempt is still running: no duplicate kicks.
    wifi_manager_supervise(1000 + 10 * WIFI_WATCH_MAX_RETRY_MS);
    EXPECT_EQ(connect_calls, before + 1);
    no_response = false;
    queue(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disc, sizeof(disc)); drain();
    EXPECT_TRUE(wifi_manager_is_connected());
    EXPECT_EQ(stop_calls, stops);  // recovered without stopping the radio
}
TEST_F(ReadPicoWifiManager, SupervisorLeavesProvisioningAndStoppedStationAlone) {
    no_response = true;
    ASSERT_EQ(wifi_manager_connect("old", "password"), ESP_ERR_TIMEOUT);
    wifi_manager_stop_connecting();
    auto before = connect_calls;
    for (int64_t t = 0; t <= 2 * WIFI_WATCH_MAX_RETRY_MS; t += 1000) wifi_manager_supervise(t);
    EXPECT_EQ(connect_calls, before);
}
TEST_F(ReadPicoWifiManager, PerformanceSettingDoesNotReuseStaleCacheOrHideSdkFailure) {
    ASSERT_EQ(wifi_manager_set_performance_mode(true), ESP_OK);
    EXPECT_EQ(last_ps, WIFI_PS_NONE);
    ASSERT_EQ(wifi_manager_connect("new", "password"), ESP_OK);
    EXPECT_EQ(last_ps, WIFI_PS_MIN_MODEM);
    ASSERT_EQ(wifi_manager_set_performance_mode(true), ESP_OK);
    EXPECT_EQ(last_ps, WIFI_PS_NONE); auto before = ps_calls;
    ps_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(wifi_manager_set_performance_mode(true), ESP_ERR_NO_MEM);
    EXPECT_EQ(ps_calls, before + 1);
}
TEST_F(ReadPicoWifiManager, FailedPowerSaveSetupCannotClaimOrdinaryConnectionSuccess) {
    ps_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_connect("new", "password"), ESP_FAIL);
    EXPECT_FALSE(wifi_manager_is_connected()); EXPECT_FALSE(running);
}
TEST_F(ReadPicoWifiManager, ExplicitCandidateUsesItsStaticFieldsAndDnsNotConfiguredValues) {
    static_ip = "10.0.0.42"; dns_override = "1.1.1.1";
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    ASSERT_EQ(wifi_manager_test_network("new", "password", &network), ESP_OK);
    EXPECT_EQ(parsed_ips, (std::vector<std::string>{"192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"}));
    EXPECT_EQ(last_dns_text, "8.8.8.8"); EXPECT_EQ(static_ip, "10.0.0.42"); EXPECT_EQ(dns_override, "1.1.1.1");
}
TEST_F(ReadPicoWifiManager, ExplicitDhcpCanTestRecoveryFromInvalidConfiguredStaticValues) {
    static_ip = "bad"; dns_override = "bad";
    wifi_network_t network{};
    ASSERT_EQ(wifi_manager_test_network("new", "password", &network), ESP_OK);
    EXPECT_EQ(dns_calls, 0); EXPECT_TRUE(parsed_ips.empty());
    EXPECT_EQ(static_ip, "bad"); EXPECT_EQ(dns_override, "bad");
}
TEST_F(ReadPicoWifiManager, CandidateFailureKeepsConfiguredValuesAndRestoresAp) {
    static_ip = "10.0.0.42"; dns_override = "1.1.1.1"; no_response = true;
    wifi_network_t network{};
    EXPECT_EQ(wifi_manager_test_network("new", "password", &network), ESP_ERR_TIMEOUT);
    EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_EQ(static_ip, "10.0.0.42"); EXPECT_EQ(dns_override, "1.1.1.1");
    EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, CandidateSnapshotSurvivesCallerAndConfiguredValueChangesBeforeIp) {
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    mutate_at_wait = &network; change_config_at_wait = true;
    ASSERT_EQ(wifi_manager_test_network("new", "password", &network), ESP_OK);
    EXPECT_EQ(last_dns_text, "8.8.8.8"); EXPECT_TRUE(wifi_manager_is_connected());
    EXPECT_STREQ(network.dns, ""); EXPECT_EQ(dns_override, "bad");
}
TEST_F(ReadPicoWifiManager, ExplicitInvalidNetworkHasNoSdkSideEffectsOrFallbackToConfig) {
    wifi_network_t network{}; network.static_address = true;
    EXPECT_EQ(wifi_manager_test_network("new", "password", &network), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_test_network("new", "password", nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_calls, 0); EXPECT_EQ(connect_calls, 0); EXPECT_EQ(fence_calls, 0); EXPECT_EQ(stop_calls, 0);
}
TEST_F(ReadPicoWifiManager, OwnedCandidateCanBeCanceledWithoutStoppingProvisioningAp) {
    wifi_network_t network{}; uint32_t owner = 0;
    ASSERT_EQ(wifi_manager_test_network_owned("new", "password", &network, &owner), ESP_OK);
    ASSERT_NE(owner, 0u); ASSERT_TRUE(wifi_manager_is_connected());
    EXPECT_EQ(wifi_manager_cancel_candidate(owner), ESP_OK);
    EXPECT_EQ(mode, WIFI_MODE_AP); EXPECT_TRUE(running); EXPECT_EQ(stop_calls, 0);
    EXPECT_FALSE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, LateCleanupCannotCancelNewerSameSsidCandidateAndFailureDoesNotPublishOwner) {
    wifi_network_t network{}; uint32_t old_owner = 0, current_owner = 0;
    ASSERT_EQ(wifi_manager_test_network_owned("new", "password", &network, &old_owner), ESP_OK);
    ASSERT_EQ(wifi_manager_test_network_owned("new", "password", &network, &current_owner), ESP_OK);
    EXPECT_NE(old_owner, current_owner);
    EXPECT_EQ(wifi_manager_cancel_candidate(old_owner), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(wifi_manager_is_connected()); EXPECT_EQ(mode, WIFI_MODE_APSTA);
    network.static_address = true;
    EXPECT_EQ(wifi_manager_test_network_owned("new", "password", &network, &old_owner), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_manager_test_network_owned("new", "password", &network, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_TRUE(wifi_manager_is_connected());
}
TEST_F(ReadPicoWifiManager, CandidateCleanupSdkFailureIsNotReportedAsStationOffOrReadiness) {
    wifi_network_t network{}; uint32_t owner = 0;
    ASSERT_EQ(wifi_manager_test_network_owned("new", "password", &network, &owner), ESP_OK);
    mode_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_cancel_candidate(owner), ESP_FAIL);
    EXPECT_EQ(mode, WIFI_MODE_APSTA); EXPECT_FALSE(wifi_manager_is_connected());
    EXPECT_EQ(wifi_manager_cancel_candidate(owner), ESP_ERR_INVALID_STATE);
}
