#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "wifi_provisioning_ap.h"
#include "wifi_manager.h"
#include "provisioning_session.h"
#include "dns_server.h"
#include "freertos/task.h"
#include "read_pico_http_service.h"
}
namespace {
esp_netif_t ap{};
std::vector<std::string> calls;
std::string fail_at;
esp_err_t fail_code = ESP_FAIL;
esp_err_t dns_stop_error, http_stop_error, cleanup_radio_error;
bool initialized = true, bad_http_handle, null_http_handle, nested, mutate_name;
bool probe_frame_claim;
char mutable_name[33] = "test-ap";
uint32_t old_ticket;
unsigned radio_stops, dns_stops, http_stops;
bool manager_held;
uint32_t manager_serial, manager_owner;
esp_err_t manager_begin_error, manager_quiesce_error, manager_publish_error;
std::vector<wifi_provisioning_outcome_t> manager_outcomes;
wifi_config_t captured{};
httpd_handle_t const server = reinterpret_cast<void *>(0x1234);
esp_err_t step(const char *name) {
    if (std::strcmp(name, "netif")) EXPECT_TRUE(manager_held) << name;
    calls.emplace_back(name);
    return fail_at == name ? fail_code : ESP_OK;
}
esp_err_t routes(httpd_handle_t handle) {
    EXPECT_EQ(handle, server);
    EXPECT_FALSE(wifi_provisioning_ap_active());
    EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_FALSE(provisioning_session_commit(old_ticket));
    if (nested) {
        auto before = calls.size();
        EXPECT_EQ(wifi_provisioning_ap_start("duplicate", routes), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(calls.size(), before);
    }
    return step("routes");
}
class ReadPicoProvisioningAP : public testing::Test {
    void SetUp() override {
        calls.clear(); fail_at.clear(); fail_code = ESP_FAIL;
        dns_stop_error = http_stop_error = cleanup_radio_error = ESP_OK;
        initialized = true; bad_http_handle = null_http_handle = nested = mutate_name = false;
        probe_frame_claim = false;
        radio_stops = dns_stops = http_stops = 0; captured = {};
        manager_held = false; manager_serial = manager_owner = 0;
        manager_begin_error = manager_quiesce_error = manager_publish_error = ESP_OK;
        manager_outcomes.clear();
        old_ticket = provisioning_session_begin();
        // Lifecycle cases run in separate CTest processes, including fault latches.
    }
};
bool contains(const char *name) {
    for (auto &call : calls) if (call == name) return true;
    return false;
}
}
extern "C" {
const char *esp_err_to_name(esp_err_t) { return "synthetic-error"; }
esp_err_t wifi_manager_provisioning_begin(uint32_t *owner, esp_netif_t **netif) {
    step("netif");
    if (!initialized || manager_owner) return ESP_ERR_INVALID_STATE;
    if (manager_begin_error) return manager_begin_error;
    manager_held = true; manager_owner = ++manager_serial;
    *owner = manager_owner; *netif = &ap; return ESP_OK;
}
esp_err_t wifi_manager_provisioning_quiesce(uint32_t owner) {
    EXPECT_EQ(owner, manager_owner); EXPECT_NE(owner, 0u);
    if (manager_quiesce_error) return manager_quiesce_error;
    manager_held = true; return ESP_OK;
}
esp_err_t wifi_manager_provisioning_finish(uint32_t owner, wifi_provisioning_outcome_t outcome) {
    EXPECT_EQ(owner, manager_owner); EXPECT_NE(owner, 0u);
    manager_outcomes.push_back(outcome);
    if (outcome != WIFI_PROVISION_FAILED) EXPECT_TRUE(manager_held);
    if (outcome == WIFI_PROVISION_READY && manager_publish_error) return manager_publish_error;
    manager_held = false;
    if (outcome == WIFI_PROVISION_STOPPED) manager_owner = 0;
    return ESP_OK;
}
esp_err_t esp_wifi_stop(void) {
    EXPECT_TRUE(manager_held);
    if (probe_frame_claim) EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_ERR_INVALID_STATE);
    ++radio_stops;
    if (mutate_name) std::strcpy(mutable_name, "changed-after-stop");
    if (radio_stops > 1) { calls.emplace_back("radio-close"); return cleanup_radio_error; }
    if (nested && calls.empty()) EXPECT_FALSE(wifi_provisioning_ap_active());
    return step("initial-stop");
}
esp_err_t esp_wifi_set_mode(wifi_mode_t mode) { EXPECT_EQ(mode, WIFI_MODE_AP); return step("mode"); }
esp_err_t esp_wifi_set_config(int interface, const wifi_config_t *config) {
    EXPECT_EQ(interface, WIFI_IF_AP); captured = *config;
    EXPECT_EQ(config->ap.channel, 1); EXPECT_EQ(config->ap.max_connection, 4);
    EXPECT_EQ(config->ap.authmode, WIFI_AUTH_OPEN); EXPECT_EQ(config->ap.password[0], 0);
    return step("config");
}
esp_err_t esp_wifi_start(void) { return step("radio-start"); }
void vTaskDelay(TickType_t ticks) { EXPECT_EQ(ticks, pdMS_TO_TICKS(100)); step("delay"); }
esp_err_t esp_netif_dhcps_stop(esp_netif_t *netif) { EXPECT_EQ(netif, &ap); return step("dhcp-stop"); }
esp_err_t esp_netif_dhcps_start(esp_netif_t *netif) { EXPECT_EQ(netif, &ap); return step("dhcp-start"); }
esp_err_t esp_netif_set_ip_info(esp_netif_t *netif, const esp_netif_ip_info_t *ip) {
    EXPECT_EQ(netif, &ap); EXPECT_EQ(ip->ip.addr, 0x0104a8c0u); EXPECT_EQ(ip->gw.addr, 0x0104a8c0u);
    EXPECT_EQ(ip->netmask.addr, 0x00ffffffu); return step("ip");
}
esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config) {
    EXPECT_EQ(*handle, nullptr); EXPECT_EQ(config->server_port, 80u);
    EXPECT_EQ(config->max_uri_handlers, 16u);
    auto error = step("http-start");
    if ((error == ESP_OK && !null_http_handle) || bad_http_handle) *handle = server;
    return error;
}
esp_err_t httpd_stop(httpd_handle_t handle) {
    EXPECT_TRUE(manager_held);
    EXPECT_EQ(handle, server); ++http_stops; calls.emplace_back("http-close");
    EXPECT_FALSE(wifi_provisioning_ap_active());
    EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_FALSE(provisioning_session_commit(old_ticket));
    if (nested) {
        EXPECT_EQ(wifi_provisioning_ap_start("duplicate", routes), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
    }
    return http_stop_error;
}
esp_err_t dns_server_start(void) {
    EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_FALSE(wifi_provisioning_ap_active()); return step("dns-start");
}
esp_err_t dns_server_stop_checked(void) {
    EXPECT_TRUE(manager_held);
    ++dns_stops; calls.emplace_back("dns-close");
    EXPECT_FALSE(wifi_provisioning_ap_active()); return dns_stop_error;
}
}
TEST_F(ReadPicoProvisioningAP, SuccessPublishesOnlyAfterRoutesAndDnsAndStopsInOwnedOrder) {
    ASSERT_EQ(wifi_provisioning_ap_start("test-ap", routes), ESP_OK);
    EXPECT_TRUE(wifi_provisioning_ap_active());
    auto ticket = provisioning_session_ticket(); EXPECT_NE(ticket, 0u); EXPECT_NE(ticket, old_ticket);
    EXPECT_EQ(calls, (std::vector<std::string>{"netif", "initial-stop", "mode", "config", "radio-start",
                                              "delay", "dhcp-stop", "ip", "dhcp-start", "http-start",
                                              "routes", "dns-start"}));
    old_ticket = ticket; ASSERT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_FALSE(wifi_provisioning_ap_active()); EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_EQ(calls[calls.size()-3], "dns-close"); EXPECT_EQ(calls[calls.size()-2], "http-close");
    EXPECT_EQ(calls.back(), "radio-close");
    auto count = calls.size(); EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK); EXPECT_EQ(calls.size(), count);
    EXPECT_EQ(wifi_provisioning_ap_start("next", routes), ESP_OK);
    EXPECT_NE(provisioning_session_ticket(), ticket);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, IdleStopDoesNotPowerDownAnUnrelatedStation) {
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK); EXPECT_TRUE(calls.empty());
}
TEST_F(ReadPicoProvisioningAP, FrameHttpReservationRejectsApBeforeManagerOrAnyRadioChange) {
    ASSERT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(manager_owner, 0u); EXPECT_EQ(provisioning_session_ticket(), old_ticket);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK); // Does not release the frame's reservation.
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_PROVISION), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_http_release(READ_PICO_HTTP_FRAME), ESP_OK);
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, ApRetainsHttpPortUntilRadioExitAndOnlyThenAllowsFrameClaim) {
    probe_frame_claim = true;
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_TRUE(read_pico_http_active(READ_PICO_HTTP_PROVISION));
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_ERR_INVALID_STATE);
    cleanup_radio_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_FAIL);
    EXPECT_FALSE(read_pico_http_active(READ_PICO_HTTP_PROVISION));
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_ERR_INVALID_STATE);
    cleanup_radio_error = ESP_OK; ASSERT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, BusyManagerDoesNotAcquireSdkObjectsOrCancelExistingSession) {
    manager_begin_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(radio_stops, 0u); EXPECT_EQ(provisioning_session_ticket(), old_ticket);
    EXPECT_FALSE(manager_held); EXPECT_EQ(manager_owner, 0u); EXPECT_TRUE(manager_outcomes.empty());
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_FRAME), ESP_OK);
    EXPECT_EQ(read_pico_http_release(READ_PICO_HTTP_FRAME), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, BusyCandidateCancelsSessionButRetainsResourcesUntilExplicitStop) {
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    old_ticket = provisioning_session_ticket(); auto owner = manager_owner;
    EXPECT_FALSE(manager_held); manager_quiesce_error = ESP_ERR_INVALID_STATE;
    auto sdk_calls = calls.size();
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls.size(), sdk_calls); EXPECT_EQ(dns_stops, 0u); EXPECT_EQ(http_stops, 0u);
    EXPECT_FALSE(provisioning_session_commit(old_ticket)); EXPECT_EQ(manager_owner, owner);
    EXPECT_EQ(manager_outcomes.back(), WIFI_PROVISION_FAILED); EXPECT_FALSE(manager_held);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    manager_quiesce_error = ESP_OK;
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK); EXPECT_EQ(manager_owner, 0u); EXPECT_FALSE(manager_held);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 2u);
}
TEST_F(ReadPicoProvisioningAP, FailedReadyPublicationCleansOwnedObjectsAndReleasesReservation) {
    manager_publish_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(manager_outcomes, (std::vector<wifi_provisioning_outcome_t>{WIFI_PROVISION_READY, WIFI_PROVISION_STOPPED}));
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 2u);
    EXPECT_FALSE(manager_held); EXPECT_EQ(manager_owner, 0u); EXPECT_EQ(provisioning_session_ticket(), 0u);
    manager_publish_error = ESP_OK;
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_FALSE(manager_held); EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, StartupFailureReleasesOwnMutexButRetainsFaultLease) {
    fail_at = "routes"; http_stop_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_FALSE(manager_held); EXPECT_NE(manager_owner, 0u);
    EXPECT_EQ(manager_outcomes.back(), WIFI_PROVISION_FAILED);
    auto owner = manager_owner; auto outcomes = manager_outcomes.size();
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(manager_owner, owner); EXPECT_EQ(manager_outcomes.size(), outcomes);
    http_stop_error = ESP_OK; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_FALSE(manager_held); EXPECT_EQ(manager_owner, 0u);
}
TEST_F(ReadPicoProvisioningAP, InvalidInputsDoNotTouchSdkOrReplaceSession) {
    EXPECT_EQ(wifi_provisioning_ap_start(nullptr, routes), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_provisioning_ap_start("name", nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(wifi_provisioning_ap_start("", routes), ESP_ERR_INVALID_SIZE);
    std::string long_name(33, 'a');
    EXPECT_EQ(wifi_provisioning_ap_start(long_name.c_str(), routes), ESP_ERR_INVALID_SIZE);
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(provisioning_session_ticket(), old_ticket);
}
TEST_F(ReadPicoProvisioningAP, UninitializedOrFaultedManagerDoesNotStopForeignRadio) {
    initialized = false;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls, (std::vector<std::string>{"netif"})); EXPECT_EQ(radio_stops, 0u);
    initialized = true;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, Full32ByteSsidAndImmutableCopyKeepOriginalConfiguration) {
    std::string name(32, 's'); ASSERT_EQ(wifi_provisioning_ap_start(name.c_str(), routes), ESP_OK);
    EXPECT_EQ(captured.ap.ssid_len, 32); EXPECT_EQ(std::memcmp(captured.ap.ssid, name.data(), 32), 0);
    ASSERT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    mutate_name = true; ASSERT_EQ(wifi_provisioning_ap_start(mutable_name, routes), ESP_OK);
    EXPECT_EQ(captured.ap.ssid_len, 7); EXPECT_EQ(std::memcmp(captured.ap.ssid, "test-ap", 7), 0);
    EXPECT_STREQ(mutable_name, "changed-after-stop");
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, InitialStopFailureLatchesWithoutSecondStopOrHttpGuess) {
    fail_at = "initial-stop";
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_EQ(radio_stops, 1u); EXPECT_FALSE(contains("mode"));
    EXPECT_EQ(provisioning_session_ticket(), 0u); EXPECT_FALSE(wifi_provisioning_ap_active());
    fail_at.clear(); auto count = calls.size();
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls.size(), count);
}
TEST_F(ReadPicoProvisioningAP, NotStartedAndDhcpAlreadyStoppedAreTheOnlyAcceptedStopExceptions) {
    fail_at = "initial-stop"; fail_code = ESP_ERR_WIFI_NOT_STARTED;
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK); ASSERT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    fail_at = "dhcp-stop"; fail_code = ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED;
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK); EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, EveryRadioAndDhcpStartupFailureStopsOnlyAcquiredResources) {
    for (const char *stage : {"mode", "config", "radio-start", "dhcp-stop", "ip", "dhcp-start"}) {
        calls.clear(); radio_stops = dns_stops = http_stops = 0; fail_at = stage;
        EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL) << stage;
        EXPECT_EQ(calls.back(), "radio-close"); EXPECT_EQ(radio_stops, 2u);
        EXPECT_EQ(dns_stops, 0u); EXPECT_EQ(http_stops, 0u); EXPECT_FALSE(contains("http-start"));
        EXPECT_FALSE(wifi_provisioning_ap_active()); EXPECT_EQ(provisioning_session_ticket(), 0u);
        auto count = calls.size(); EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK); EXPECT_EQ(calls.size(), count);
    }
    fail_at.clear(); ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, HttpStartFailureDoesNotStopUnpublishedHandleOrDns) {
    fail_at = "http-start";
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_EQ(http_stops, 0u); EXPECT_EQ(dns_stops, 0u); EXPECT_EQ(radio_stops, 2u);
    fail_at.clear(); ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, RoutesFailurePreservesPrimaryErrorAndClosesHttpBeforeRadio) {
    fail_at = "routes"; fail_code = ESP_ERR_NO_MEM;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_NO_MEM);
    EXPECT_EQ(dns_stops, 0u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 2u);
    EXPECT_EQ(calls[calls.size()-2], "http-close"); EXPECT_EQ(calls.back(), "radio-close");
}
TEST_F(ReadPicoProvisioningAP, DnsStartFailureIsAcknowledgedBeforeHttpAndRadioCleanup) {
    fail_at = "dns-start";
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 2u);
    EXPECT_EQ(calls[calls.size()-3], "dns-close");
    fail_at.clear(); EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, FailedDnsStartupWithUnconfirmedCloseCannotDiscardHttpAndRadio) {
    fail_at = "dns-start"; dns_stop_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 0u); EXPECT_EQ(radio_stops, 1u);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    // The real DNS close-failure latch cannot be magically repaired by a retry.
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_FAIL);
    EXPECT_EQ(dns_stops, 2u); EXPECT_EQ(http_stops, 0u); EXPECT_EQ(radio_stops, 1u);
}
TEST_F(ReadPicoProvisioningAP, DnsStopTimeoutRetainsHttpRadioAndBlocksStartUntilExplicitAcknowledgement) {
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    old_ticket = provisioning_session_ticket(); dns_stop_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(http_stops, 0u); EXPECT_EQ(radio_stops, 1u);
    EXPECT_FALSE(provisioning_session_commit(old_ticket)); EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    dns_stop_error = ESP_OK; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_EQ(dns_stops, 2u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 2u);
}
TEST_F(ReadPicoProvisioningAP, HttpStopFailureRetainsExactHandleAndDoesNotRepeatDnsClose) {
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK); http_stop_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_FAIL);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 1u);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    http_stop_error = ESP_OK; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 2u); EXPECT_EQ(radio_stops, 2u);
}
TEST_F(ReadPicoProvisioningAP, RadioStopFailureRetainsOwnershipWithoutRepeatingClosedServices) {
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK); cleanup_radio_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_FAIL);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    cleanup_radio_error = ESP_ERR_WIFI_NOT_STARTED; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_EQ(dns_stops, 1u); EXPECT_EQ(http_stops, 1u); EXPECT_EQ(radio_stops, 3u);
}
TEST_F(ReadPicoProvisioningAP, StartupCleanupFailureRetainsPrimaryFailureAndRequiresExplicitStop) {
    fail_at = "routes"; fail_code = ESP_ERR_NO_MEM; http_stop_error = ESP_FAIL;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_NO_MEM);
    EXPECT_EQ(radio_stops, 1u); EXPECT_EQ(http_stops, 1u);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    http_stop_error = ESP_OK; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_EQ(http_stops, 2u); EXPECT_EQ(radio_stops, 2u);
}
TEST_F(ReadPicoProvisioningAP, HttpErrorWithPublishedHandleLatchesWithoutGuessingCleanupContract) {
    fail_at = "http-start"; bad_http_handle = true;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_FAIL);
    EXPECT_EQ(http_stops, 0u); EXPECT_EQ(radio_stops, 1u);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoProvisioningAP, HttpSuccessWithoutHandleIsNotPublishedAsReady) {
    null_http_handle = true;
    EXPECT_EQ(wifi_provisioning_ap_start("name", routes), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(wifi_provisioning_ap_active()); EXPECT_EQ(provisioning_session_ticket(), 0u);
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoProvisioningAP, NestedOrDuplicateStartAndStopCannotReplaceOrReleaseServiceObjects) {
    nested = true; ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    auto count = calls.size(); auto ticket = provisioning_session_ticket();
    EXPECT_EQ(wifi_provisioning_ap_start("duplicate", routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls.size(), count); EXPECT_EQ(provisioning_session_ticket(), ticket);
    old_ticket = ticket; EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
TEST_F(ReadPicoProvisioningAP, CompletedSessionIsInvalidatedOnStopAndNotReusedOnNextStart) {
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    auto saved = provisioning_session_ticket(); ASSERT_TRUE(provisioning_session_commit(saved));
    ASSERT_TRUE(provisioning_session_complete()); ASSERT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
    EXPECT_FALSE(provisioning_session_complete()); EXPECT_FALSE(provisioning_session_commit(saved));
    ASSERT_EQ(wifi_provisioning_ap_start("name", routes), ESP_OK);
    EXPECT_NE(provisioning_session_ticket(), saved); EXPECT_FALSE(provisioning_session_complete());
    EXPECT_EQ(wifi_provisioning_ap_stop(), ESP_OK);
}
