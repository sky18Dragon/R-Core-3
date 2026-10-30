#include <gtest/gtest.h>
#include <functional>
#include <string>
#include <vector>
extern "C" {
#include "read_pico_sntp.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "lwip/dns.h"
#include "lwip/apps/sntp.h"
#include "lwip/udp.h"
#include "freertos/task.h"
void __wrap_sntp_get_system_time(uint32_t *, uint32_t *);
struct udp_pcb *__wrap_udp_new_ip_type(u8_t);
}
namespace {
bool connected, gate, enabled, init_failure, stop_failure, core;
bool skipped_core, after_callback_error;
int exec_calls, dns_calls, init_calls, stop_calls, clock_calls, configured;
int fail_operation, clock_error;
bool pcb_failure;
udp_pcb pcb;
int pcb_allocations, pcb_removals;
esp_err_t exec_error;
err_t dns_error;
ip_addr_t resolved_address, configured_address;
const char *configured_name;
TickType_t ticks;
uint32_t wall_seconds, wall_us, sent_seconds, sent_us;
dns_found_callback pending_callback;
void *pending_argument;
std::string queried_name;
std::vector<std::string> operations;
std::function<void()> delay_hook, init_hook, exec_hook, close_hook;
bool allowed() { return connected; }
bool clock_gate() { return gate; }
void reply(time_t seconds = 1800000000, long us = 12345) {
    timeval tv{seconds, (suseconds_t)us}; bool previous = core; core = true;
    sntp_sync_time(&tv); core = previous;
}
void finish_dns(const ip_addr_t *address = nullptr, void *ticket = nullptr) {
    ASSERT_NE(pending_callback, nullptr);
    bool previous = core; core = true;
    pending_callback(queried_name.c_str(), address ? address : &resolved_address,
                     ticket ? ticket : pending_argument);
    core = previous;
}
esp_err_t sync(time_t *out) { return read_pico_sntp_sync("clock.example", allowed, clock_gate, out); }
class ReadPicoSntp : public testing::Test {
protected:
    void SetUp() override {
        connected = gate = true; enabled = init_failure = stop_failure = core = false;
        skipped_core = after_callback_error = false;
        exec_calls = dns_calls = init_calls = stop_calls = clock_calls = configured = 0;
        fail_operation = 0; clock_error = 0; exec_error = ESP_FAIL; dns_error = ERR_OK;
        pcb_failure = false; pcb_allocations = pcb_removals = 0;
        resolved_address = {0x01020304}; configured_address = {0}; configured_name = nullptr;
        ticks = 0; wall_seconds = 1800000000; wall_us = 10; sent_seconds = sent_us = 0;
        pending_callback = nullptr; pending_argument = nullptr;
        queried_name.clear(); operations.clear();
        delay_hook = init_hook = exec_hook = close_hook = nullptr;
    }
};
}
extern "C" {
esp_err_t esp_netif_tcpip_exec(esp_netif_callback_fn fn, void *arg) {
    ++exec_calls; int op = (int)(uintptr_t)arg;
    if (exec_hook) { auto hook = std::move(exec_hook); exec_hook = nullptr; hook(); }
    if (fail_operation == op && !after_callback_error) return skipped_core ? ESP_OK : exec_error;
    core = true; esp_err_t result = fn(arg); core = false;
    if (fail_operation == op && after_callback_error) return exec_error;
    return result;
}
err_t dns_gethostbyname_addrtype(const char *name, ip_addr_t *out, dns_found_callback cb, void *arg, uint8_t type) {
    EXPECT_TRUE(core); EXPECT_EQ(type, LWIP_DNS_ADDRTYPE_DEFAULT); ++dns_calls;
    queried_name = name; pending_callback = cb; pending_argument = arg; operations.push_back("resolve");
    if (dns_error == ERR_OK) *out = resolved_address;
    return dns_error;
}
TickType_t xTaskGetTickCount(void) { return ticks; }
void vTaskDelay(TickType_t delay) {
    EXPECT_FALSE(core); ticks += delay;
    if (delay_hook) { auto hook = std::move(delay_hook); delay_hook = nullptr; hook(); }
}
void sntp_setoperatingmode(uint8_t mode) { EXPECT_TRUE(core); EXPECT_EQ(mode, SNTP_OPMODE_POLL); }
void sntp_set_sync_mode(sntp_sync_mode_t mode) { EXPECT_TRUE(core); EXPECT_EQ(mode, SNTP_SYNC_MODE_IMMED); }
void sntp_set_sync_status(sntp_sync_status_t status) { EXPECT_TRUE(core); EXPECT_EQ(status, SNTP_SYNC_STATUS_COMPLETED); }
void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t cb) { EXPECT_TRUE(core); EXPECT_EQ(cb, nullptr); }
void sntp_setserver(uint8_t i, const ip_addr_t *a) {
    EXPECT_TRUE(core); EXPECT_EQ(i, 0); configured_address = *a; ++configured;
}
const ip_addr_t *sntp_getserver(uint8_t i) { EXPECT_TRUE(core); EXPECT_EQ(i, 0); return &configured_address; }
void sntp_setservername(uint8_t i, const char *name) {
    EXPECT_TRUE(core); EXPECT_EQ(i, 0); EXPECT_EQ(name, nullptr); configured_name = name;
}
const char *sntp_getservername(uint8_t i) { EXPECT_TRUE(core); EXPECT_EQ(i, 0); return configured_name; }
uint8_t sntp_enabled(void) { EXPECT_TRUE(core); return enabled; }
void sntp_init(void) {
    EXPECT_TRUE(core); ++init_calls; operations.push_back("start");
    EXPECT_EQ(__wrap_udp_new_ip_type(IPADDR_TYPE_ANY), &pcb);
    enabled = !init_failure;
    if (enabled) __wrap_sntp_get_system_time(&sent_seconds, &sent_us);
    if (init_hook) { auto hook = std::move(init_hook); init_hook = nullptr; hook(); }
}
void sntp_stop(void) {
    EXPECT_TRUE(core); ++stop_calls; operations.push_back("stop");
    if (close_hook) { auto hook = std::move(close_hook); close_hook = nullptr; hook(); }
    if (!stop_failure) enabled = false;
}
udp_pcb *__real_udp_new_ip_type(u8_t type) {
    EXPECT_TRUE(core); EXPECT_EQ(type, IPADDR_TYPE_ANY); ++pcb_allocations;
    return pcb_failure ? nullptr : &pcb;
}
void udp_remove(udp_pcb *p) { EXPECT_TRUE(core); EXPECT_EQ(p, &pcb); ++pcb_removals; }
void __real_sntp_get_system_time(uint32_t *seconds, uint32_t *us) { *seconds = wall_seconds; *us = wall_us; }
int read_pico_sntp_test_settimeofday(const struct timeval *tv, const struct timezone *) {
    EXPECT_TRUE(core); ++clock_calls; EXPECT_NE(tv, nullptr); return clock_error;
}
}

TEST_F(ReadPicoSntp, ValidReplyAppliesOnceAndPublishesAfterNumericOwnedStop) {
    init_hook = [] { reply(); reply(1800000010); }; time_t out = 42;
    ASSERT_EQ(sync(&out), ESP_OK); EXPECT_EQ(out, 1800000000);
    EXPECT_EQ(clock_calls, 1); EXPECT_EQ(stop_calls, 1); EXPECT_FALSE(enabled);
    EXPECT_EQ(configured_address.value, 0u); EXPECT_EQ(configured_name, nullptr);
    EXPECT_EQ(operations, (std::vector<std::string>{"resolve", "start", "stop"}));
}
TEST_F(ReadPicoSntp, AllocationFailureDoesNotConfigureStartStopOrAbortAndAllowsExplicitRetry) {
    pcb_failure = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_NO_MEM); EXPECT_EQ(init_calls, 0); EXPECT_EQ(stop_calls, 0);
    EXPECT_EQ(configured, 0); EXPECT_EQ(out, 42); EXPECT_EQ(pcb_allocations, 1);
    pcb_failure = false; init_hook = [] { reply(); }; EXPECT_EQ(sync(&out), ESP_OK);
}
TEST_F(ReadPicoSntp, OtherUdpAllocationUsesOriginalAllocatorWithoutTakingSntpReservation) {
    core = true;
    EXPECT_EQ(__wrap_udp_new_ip_type(IPADDR_TYPE_ANY), &pcb);
    core = false; EXPECT_EQ(pcb_allocations, 1); EXPECT_EQ(init_calls, 0); EXPECT_EQ(configured, 0);
}
TEST_F(ReadPicoSntp, SnapshotDoesNotBorrowMutableRequestDuringResolveOrService) {
    char request[] = "clock.example"; exec_hook = [&] { request[0] = 'X'; };
    init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(read_pico_sntp_sync(request, allowed, clock_gate, &out), ESP_OK);
    EXPECT_EQ(queried_name, "clock.example"); EXPECT_EQ(configured_name, nullptr);
}
TEST_F(ReadPicoSntp, InvalidArgumentsHaveNoIoOrOutputChanges) {
    time_t out = 42;
    EXPECT_EQ(read_pico_sntp_sync(nullptr, allowed, clock_gate, &out), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_sntp_sync("clock", nullptr, clock_gate, &out), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_sntp_sync("clock", allowed, nullptr, &out), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_sntp_sync("clock", allowed, clock_gate, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_sntp_sync("", allowed, clock_gate, &out), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(read_pico_sntp_sync(std::string(128, 'x').c_str(), allowed, clock_gate, &out), ESP_ERR_INVALID_SIZE);
    for (auto bad : {"bad host", "host\n", "host\x7f", "host\xc2\xa0"})
        EXPECT_EQ(read_pico_sntp_sync(bad, allowed, clock_gate, &out), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(out, 42); EXPECT_EQ(exec_calls, 0);
}
TEST_F(ReadPicoSntp, DisconnectedDoesNotResolveOrStart) {
    connected = false; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(exec_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, StopBeforeUseClosesAdmissionWithoutTouchingForeignService) {
    enabled = true; EXPECT_EQ(read_pico_sntp_stop(), ESP_OK); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(exec_calls, 0); EXPECT_TRUE(enabled);
}
TEST_F(ReadPicoSntp, PendingDnsCompletesBeforeNumericStart) {
    dns_error = ERR_INPROGRESS; delay_hook = [] { finish_dns(); };
    init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(dns_calls, 1); EXPECT_EQ(init_calls, 1); EXPECT_EQ(ticks, 100u);
}
TEST_F(ReadPicoSntp, DnsNullFailureNeverStartsSdkAndCanExplicitlyRetry) {
    dns_error = ERR_INPROGRESS; delay_hook = [] {
        core = true; pending_callback("clock.example", nullptr, pending_argument); core = false;
    }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_NOT_FOUND); EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
    dns_error = ERR_OK; init_hook = [] { reply(); }; EXPECT_EQ(sync(&out), ESP_OK);
}
TEST_F(ReadPicoSntp, DnsMemoryFailurePropagatesNoStopOrStart) {
    dns_error = ERR_MEM; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_NO_MEM); EXPECT_EQ(init_calls, 0); EXPECT_EQ(stop_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, DnsArgumentFailurePropagatesNoSuccess) {
    dns_error = ERR_ARG; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_FAIL); EXPECT_EQ(out, 42); EXPECT_EQ(init_calls, 0);
}
TEST_F(ReadPicoSntp, AnyOrMulticastDnsAddressRejectedBeforeSdkStart) {
    time_t out = 42;
    for (uint32_t a : {0u, 0xe1020304u}) {
        resolved_address.value = a; EXPECT_EQ(sync(&out), ESP_ERR_NOT_FOUND);
    }
    EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, DnsTimeoutKeepsIdentityAndLateCallbackDoesNotStartService) {
    dns_error = ERR_INPROGRESS; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_TIMEOUT); void *old = pending_argument;
    EXPECT_EQ(ticks, 10000u); EXPECT_EQ(sync(&out), ESP_ERR_NOT_FINISHED); EXPECT_EQ(dns_calls, 1);
    finish_dns(); EXPECT_EQ(init_calls, 0); EXPECT_EQ(clock_calls, 0);
    dns_error = ERR_OK; init_hook = [old] { finish_dns(nullptr, old); reply(); };
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_NE(old, pending_argument); EXPECT_EQ(dns_calls, 2);
}
TEST_F(ReadPicoSntp, CancelDuringDnsRetainsPendingUntilItsOwnCallback) {
    dns_error = ERR_INPROGRESS; delay_hook = [] { read_pico_sntp_request_stop(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(out, 42);
    EXPECT_EQ(read_pico_sntp_stop(), ESP_ERR_NOT_FINISHED); finish_dns();
    EXPECT_EQ(read_pico_sntp_stop(), ESP_OK); EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(init_calls, 0); EXPECT_EQ(stop_calls, 0);
}
TEST_F(ReadPicoSntp, DnsCompletionExitRequiresCoreFenceAndCannotIgnoreMissingAck) {
    dns_error = ERR_INPROGRESS; delay_hook = [] { read_pico_sntp_request_stop(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); finish_dns();
    fail_operation = 3; skipped_core = true;
    EXPECT_EQ(read_pico_sntp_stop(), ESP_ERR_INVALID_RESPONSE);
    EXPECT_EQ(stop_calls, 0); EXPECT_EQ(init_calls, 0);
    fail_operation = 0; EXPECT_EQ(read_pico_sntp_stop(), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoSntp, WrongDnsTicketCannotCompleteCurrentRequest) {
    dns_error = ERR_INPROGRESS; delay_hook = [] { finish_dns(nullptr, (void *)99); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_TIMEOUT); EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, DisconnectDuringDnsDoesNotDiscardOrReusePendingIdentity) {
    dns_error = ERR_INPROGRESS; delay_hook = [] { connected = false; }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); connected = true;
    EXPECT_EQ(sync(&out), ESP_ERR_NOT_FINISHED); EXPECT_EQ(dns_calls, 1); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, CoreSuccessWithoutCallbackAckIsFaultNotSuccessOrReuse) {
    fail_operation = 1; skipped_core = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(dns_calls, 0); EXPECT_EQ(out, 42);
    fail_operation = 0; EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
    EXPECT_NE(read_pico_sntp_stop(), ESP_OK);
}
TEST_F(ReadPicoSntp, ResolveCoreErrorIsPropagatedAndLatchesUncertain) {
    fail_operation = 1; exec_error = ESP_ERR_NO_MEM; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_NO_MEM); EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
    fail_operation = 0; EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoSntp, MissingStartAckDoesNotPretendServiceStarted) {
    fail_operation = 2; skipped_core = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
    EXPECT_NE(read_pico_sntp_stop(), ESP_OK);
}
TEST_F(ReadPicoSntp, InitWithoutEnabledServiceCleansOnlyOwnConfigurationAndLatches) {
    init_failure = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_FAIL); EXPECT_EQ(stop_calls, 1); EXPECT_EQ(configured_address.value, 0u);
    EXPECT_EQ(out, 42); init_failure = false; EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoSntp, ForeignActiveClientIsNotStoppedConfiguredOrReplaced) {
    enabled = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(init_calls, 0); EXPECT_EQ(stop_calls, 0);
    EXPECT_EQ(configured, 0); EXPECT_TRUE(enabled); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, ForeignDisabledConfiguredClientIsNotOverwritten) {
    configured_name = "foreign.example"; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(configured, 0);
    EXPECT_STREQ(configured_name, "foreign.example");
    configured_name = nullptr; configured_address.value = 1;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(configured_address.value, 1u);
}
TEST_F(ReadPicoSntp, ReplyTimeoutStopsOwnServiceAndExplicitNextAttemptCanRun) {
    time_t out = 42; EXPECT_EQ(sync(&out), ESP_ERR_TIMEOUT);
    EXPECT_EQ(stop_calls, 1); EXPECT_FALSE(enabled); EXPECT_EQ(out, 42);
    init_hook = [] { reply(); }; EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(init_calls, 2);
}
TEST_F(ReadPicoSntp, CancelAfterStartSuppressesClockAndRetainsOutput) {
    init_hook = [] { read_pico_sntp_request_stop(); reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(clock_calls, 0); EXPECT_EQ(out, 42);
    EXPECT_EQ(stop_calls, 1); EXPECT_EQ(read_pico_sntp_stop(), ESP_OK);
}
TEST_F(ReadPicoSntp, ClockGateClosedReplyDoesNotApplyOrConfirm) {
    init_hook = [] { gate = false; reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(clock_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, InvalidReplyTimeOrMicrosecondsNeverApplies) {
    time_t out = 42;
    for (auto tv : {timeval{0, 0}, timeval{1577836799, 0}, timeval{4102444800, 0},
                    timeval{1800000000, -1}, timeval{1800000000, 1000000}}) {
        init_hook = [tv] { auto copy = tv; sntp_sync_time(&copy); };
        EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE);
    }
    EXPECT_EQ(clock_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, NullReplyNeverAppliesOrPublishes) {
    init_hook = [] { sntp_sync_time(nullptr); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(out, 42); EXPECT_EQ(clock_calls, 0);
}
TEST_F(ReadPicoSntp, ClockSetFailureDoesNotBecomeConfirmedTime) {
    clock_error = -1; init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_FAIL); EXPECT_EQ(clock_calls, 1); EXPECT_EQ(out, 42); EXPECT_EQ(stop_calls, 1);
}
TEST_F(ReadPicoSntp, StopStillEnabledRetainsOwnServiceAndRefusesReuse) {
    stop_failure = true; init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_FAIL); EXPECT_TRUE(enabled); EXPECT_EQ(out, 42);
    int before = init_calls; EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(init_calls, before);
    stop_failure = false; EXPECT_NE(read_pico_sntp_stop(), ESP_OK); EXPECT_FALSE(enabled);
    EXPECT_EQ(stop_calls, 2); EXPECT_EQ(configured_address.value, 0u);
}
TEST_F(ReadPicoSntp, MissingStopAckRetainsServiceAndDoesNotPublishAppliedTime) {
    fail_operation = 3; skipped_core = true; init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_TRUE(enabled); EXPECT_EQ(out, 42);
    fail_operation = 0; EXPECT_NE(read_pico_sntp_stop(), ESP_OK); EXPECT_FALSE(enabled);
}
TEST_F(ReadPicoSntp, TimeoutAndMissingStopAckCannotApplyLateReply) {
    fail_operation = 3; skipped_core = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_TRUE(enabled);
    reply(); EXPECT_EQ(clock_calls, 0); EXPECT_EQ(out, 42);
    fail_operation = 0; EXPECT_NE(read_pico_sntp_stop(), ESP_OK); EXPECT_FALSE(enabled);
}
TEST_F(ReadPicoSntp, StopExecErrorAfterRealCallbackIsNotReportedAsSuccessfulSync) {
    fail_operation = 3; after_callback_error = true; init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_FAIL); EXPECT_FALSE(enabled); EXPECT_EQ(out, 42);
    EXPECT_NE(read_pico_sntp_stop(), ESP_OK);
}
TEST_F(ReadPicoSntp, ReentrantSyncAndStopDoNotMutateSdkWhileOuterOperationOwnsControl) {
    exec_hook = [] {
        time_t nested = 99; EXPECT_EQ(sync(&nested), ESP_ERR_INVALID_STATE); EXPECT_EQ(nested, 99);
        EXPECT_EQ(read_pico_sntp_stop(), ESP_ERR_INVALID_STATE);
    }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(init_calls, 0); EXPECT_EQ(out, 42);
}
TEST_F(ReadPicoSntp, ClockAlreadyAppliedBeforeCancellationIsNotRolledBackOrConfirmed) {
    init_hook = [] { reply(); connected = false; }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(clock_calls, 1); EXPECT_EQ(out, 42);
    EXPECT_EQ(stop_calls, 1);
}
TEST_F(ReadPicoSntp, LateApplyHookAfterStopDoesNotChangeClockOrResult) {
    init_hook = [] { reply(); }; time_t out = 42;
    ASSERT_EQ(sync(&out), ESP_OK); reply(1800000010); EXPECT_EQ(clock_calls, 1); EXPECT_EQ(out, 1800000000);
}
TEST_F(ReadPicoSntp, RequestTimestampRemainsUniqueAcrossClockBackwardsAndSameInstant) {
    init_hook = [] { reply(); }; time_t out = 42;
    ASSERT_EQ(sync(&out), ESP_OK); EXPECT_EQ(sent_us, 10u);
    wall_seconds = 1; wall_us = 0; init_hook = [] { reply(); };
    ASSERT_EQ(sync(&out), ESP_OK); EXPECT_EQ(sent_seconds, 1800000000u); EXPECT_EQ(sent_us, 11u);
    init_hook = [] { reply(); }; ASSERT_EQ(sync(&out), ESP_OK); EXPECT_EQ(sent_us, 12u);
}
TEST_F(ReadPicoSntp, TimestampExhaustionSuppressesApplyAndLatchesUntilAudit) {
    wall_seconds = UINT32_MAX; wall_us = 999999; init_hook = [] { reply(); }; time_t out = 42;
    ASSERT_EQ(sync(&out), ESP_OK); out = 42; init_hook = [] { reply(); };
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(clock_calls, 1); EXPECT_EQ(out, 42);
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoSntp, InvalidTimestampSourceDoesNotApplyAndLatches) {
    wall_us = 1000000; init_hook = [] { reply(); }; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(clock_calls, 0); EXPECT_EQ(out, 42);
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoSntp, TickWrapDoesNotExtendReplyTimeout) {
    ticks = UINT32_MAX - 500; time_t out = 42; TickType_t initial = ticks;
    EXPECT_EQ(sync(&out), ESP_ERR_TIMEOUT); EXPECT_EQ((TickType_t)(ticks - initial), 10000u);
    EXPECT_EQ(out, 42); EXPECT_EQ(stop_calls, 1);
}
