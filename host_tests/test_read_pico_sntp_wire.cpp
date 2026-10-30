#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>
extern "C" {
#include "read_pico_sntp.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "lwip/udp.h"
#include "lwip/timeouts.h"
#include "lwip/apps/sntp.h"
#include "freertos/task.h"
}
namespace {
struct Timer { uint32_t due; sys_timeout_handler callback; void *argument; };
std::vector<Timer> timers;
udp_pcb pcb;
udp_recv_fn receive_callback;
void *receive_argument;
bool core, connected = true, gate = true;
bool pcb_failure;
int allocations, removals, sends, clock_calls, dns_calls;
uint32_t ticks, wall_seconds, wall_us;
ip_addr_t server{0x01020304};
std::array<uint8_t, 48> sent;
std::function<void()> on_send;
timeval applied;
bool allowed() { return connected; }
bool clock_gate() { return gate; }
esp_err_t sync(time_t *out) { return read_pico_sntp_sync("clock.example", allowed, clock_gate, out); }
void receive(std::array<uint8_t, 48> bytes, ip_addr_t source = server, unsigned port = 123, unsigned length = 48) {
    pbuf *p = pbuf_alloc(PBUF_TRANSPORT, length, PBUF_RAM);
    memcpy(p->payload, bytes.data(), std::min(length, 48u));
    EXPECT_TRUE(core); ASSERT_NE(receive_callback, nullptr);
    receive_callback(receive_argument, &pcb, p, &source, port);
}
std::array<uint8_t, 48> response(const std::array<uint8_t, 48> &request) {
    std::array<uint8_t, 48> out{}; out[0] = 0x24; out[1] = 1;
    memcpy(out.data() + 24, request.data() + 40, 8);
    uint32_t sec = htonl((uint32_t)1800000000 + UINT32_C(2208988800));
    memcpy(out.data() + 40, &sec, 4); return out;
}
class ReadPicoSntpWire : public testing::Test {
protected:
    void SetUp() override {
        receive_callback = nullptr; receive_argument = nullptr;
        core = false; connected = gate = true;
        pcb_failure = false;
        allocations = removals = sends = clock_calls = dns_calls = 0;
        ticks = 0; wall_seconds = 1800000000; wall_us = 0;
        timers.clear(); sent.fill(0); on_send = nullptr; applied = {};
    }
    void TearDown() override { EXPECT_EQ(allocations, 0); }
};
}
extern "C" {
const ip_addr_t sntp_wire_any = {0};
esp_err_t esp_netif_tcpip_exec(esp_netif_callback_fn fn, void *argument) {
    core = true; esp_err_t result = fn(argument); core = false; return result;
}
err_t dns_gethostbyname_addrtype(const char *, ip_addr_t *out, dns_found_callback, void *, uint8_t type) {
    EXPECT_TRUE(core); EXPECT_EQ(type, LWIP_DNS_ADDRTYPE_DEFAULT); ++dns_calls; *out = server; return ERR_OK;
}
// Only our outer DNS is allowed. The SDK uses this other entry point if it
// accidentally retained a hostname; a test failure catches that regression.
err_t dns_gethostbyname(const char *, ip_addr_t *, dns_found_callback, void *) {
    ADD_FAILURE() << "SDK SNTP must not initiate DNS"; return ERR_ARG;
}
TickType_t xTaskGetTickCount(void) { return ticks; }
void vTaskDelay(TickType_t n) {
    ticks += n; core = true;
    for (;;) {
        auto it = std::find_if(timers.begin(), timers.end(), [](const Timer &t) { return t.due <= ticks; });
        if (it == timers.end()) break;
        auto timer = *it; timers.erase(it); timer.callback(timer.argument);
    }
    core = false;
}
void sys_timeout(u32_t delay, sys_timeout_handler fn, void *argument) {
    EXPECT_TRUE(core); timers.push_back({ticks + delay, fn, argument});
}
void sys_untimeout(sys_timeout_handler fn, void *argument) {
    EXPECT_TRUE(core);
    auto it = std::find_if(timers.begin(), timers.end(), [&](const Timer &t) {
        return t.callback == fn && t.argument == argument;
    });
    if (it != timers.end()) timers.erase(it);
}
udp_pcb *__real_udp_new_ip_type(u8_t) { EXPECT_TRUE(core); return pcb_failure ? nullptr : &pcb; }
udp_pcb *__wrap_udp_new_ip_type(u8_t);
udp_pcb *udp_new_ip_type(u8_t type) { return __wrap_udp_new_ip_type(type); }
void udp_recv(udp_pcb *p, udp_recv_fn fn, void *argument) {
    EXPECT_TRUE(core); EXPECT_EQ(p, &pcb); receive_callback = fn; receive_argument = argument;
}
err_t udp_sendto(udp_pcb *p, pbuf *packet, const ip_addr_t *destination, u16_t port) {
    EXPECT_TRUE(core); EXPECT_EQ(p, &pcb); EXPECT_EQ(port, 123); EXPECT_EQ(destination->value, server.value);
    EXPECT_EQ(packet->tot_len, 48); memcpy(sent.data(), packet->payload, 48); ++sends;
    // Deliver after send_request has recorded its last timestamp/address.
    return ERR_OK;
}
err_t udp_bind(udp_pcb *, const ip_addr_t *, u16_t) { ADD_FAILURE(); return ERR_ARG; }
void udp_remove(udp_pcb *p) { EXPECT_TRUE(core); EXPECT_EQ(p, &pcb); ++removals; }
pbuf *pbuf_alloc(int, u16_t length, int) {
    ++allocations; auto p = new pbuf; p->payload = calloc(1, length); p->tot_len = length; return p;
}
u8_t pbuf_free(pbuf *p) { --allocations; free(p->payload); delete p; return 1; }
u8_t pbuf_get_at(const pbuf *p, u16_t offset) { return ((uint8_t *)p->payload)[offset]; }
u16_t pbuf_copy_partial(const pbuf *p, void *out, u16_t count, u16_t offset) {
    if (offset + count > p->tot_len) { ADD_FAILURE(); return 0; }
    memcpy(out, (uint8_t *)p->payload + offset, count); return count;
}
void sntp_set_sync_mode(sntp_sync_mode_t mode) { EXPECT_TRUE(core); EXPECT_EQ(mode, SNTP_SYNC_MODE_IMMED); }
void sntp_set_sync_status(sntp_sync_status_t status) { EXPECT_TRUE(core); EXPECT_EQ(status, SNTP_SYNC_STATUS_COMPLETED); }
void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t cb) { EXPECT_TRUE(core); EXPECT_EQ(cb, nullptr); }
void __real_sntp_get_system_time(uint32_t *sec, uint32_t *us) { *sec = wall_seconds; *us = wall_us; }
int read_pico_sntp_test_settimeofday(const timeval *tv, const struct timezone *) {
    EXPECT_TRUE(core); ++clock_calls; applied = *tv; return 0;
}
void sntp_wire_set_time(uint32_t seconds, uint32_t us) {
    timeval tv{(time_t)seconds, (suseconds_t)us}; sntp_sync_time(&tv);
}
}
// Run reply delivery only AFTER each real SDK timeout callback returns.
namespace {
void arm_reply(std::function<void()> action) {
    on_send = std::move(action);
    core = true;
    sys_timeout(200, [](void *) { if (on_send) { auto action = std::move(on_send); on_send = nullptr; action(); } }, nullptr);
    core = false;
}
}
TEST_F(ReadPicoSntpWire, RealSdkAcceptsCurrentOriginateAndNumericServerThenClosesPcbAndTimers) {
    arm_reply([] { receive(response(sent)); }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(out, 1800000000); EXPECT_EQ(clock_calls, 1);
    EXPECT_EQ(sends, 1); EXPECT_EQ(dns_calls, 1); EXPECT_EQ(removals, 1); EXPECT_TRUE(timers.empty());
    EXPECT_EQ(sent[0], 0x23); EXPECT_EQ(sntp_getservername(0), nullptr); EXPECT_TRUE(ip_addr_isany(sntp_getserver(0)));
}
TEST_F(ReadPicoSntpWire, RealSdkAllocationFailureIsReturnedBeforeItsNullPcbAssertion) {
    pcb_failure = true; time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_NO_MEM); EXPECT_EQ(out, 42); EXPECT_EQ(sends, 0);
    EXPECT_EQ(removals, 0); EXPECT_TRUE(timers.empty());
    pcb_failure = false; arm_reply([] { receive(response(sent)); });
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(removals, 1);
}
TEST_F(ReadPicoSntpWire, RealSdkRejectsWrongOriginateBeforeClockAndAcceptsCurrentReply) {
    arm_reply([] {
        auto bytes = response(sent); bytes[24] ^= 1; receive(bytes); EXPECT_EQ(clock_calls, 0);
        receive(response(sent));
    }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(clock_calls, 1);
}
TEST_F(ReadPicoSntpWire, RealSdkRejectsWrongAddressOrPortBeforeClock) {
    arm_reply([] {
        receive(response(sent), ip_addr_t{0x05060708}); EXPECT_EQ(clock_calls, 0);
        receive(response(sent), server, 124); EXPECT_EQ(clock_calls, 0);
        receive(response(sent));
    }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(clock_calls, 1);
}
TEST_F(ReadPicoSntpWire, RealSdkRejectsWrongLengthModeAndKodBeforeClock) {
    arm_reply([] {
        auto bytes = response(sent); receive(bytes, server, 123, 47); EXPECT_EQ(clock_calls, 0);
        bytes[0] = 0x23; receive(bytes); EXPECT_EQ(clock_calls, 0);
        bytes = response(sent); bytes[1] = 0; receive(bytes); EXPECT_EQ(clock_calls, 0);
        receive(response(sent));
    }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(clock_calls, 1);
}
TEST_F(ReadPicoSntpWire, RealSdkLateReplyFromPreviousAttemptIsRejectedAfterClockBackwards) {
    std::array<uint8_t, 48> old;
    arm_reply([&] { old = sent; receive(response(sent)); }); time_t out = 42;
    ASSERT_EQ(sync(&out), ESP_OK); wall_seconds = 1; wall_us = 0;
    arm_reply([&] {
        EXPECT_NE(sent, old); receive(response(old)); EXPECT_EQ(clock_calls, 1);
        receive(response(sent));
    });
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(clock_calls, 2); EXPECT_EQ(removals, 2);
}
TEST_F(ReadPicoSntpWire, RealSdkRejectedReplyOnlyTimesOutWithoutPublishingOrLeakingPcb) {
    arm_reply([] { auto bytes = response(sent); bytes[31] ^= 1; receive(bytes); }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_TIMEOUT); EXPECT_EQ(out, 42); EXPECT_EQ(clock_calls, 0);
    EXPECT_EQ(removals, 1); EXPECT_TRUE(timers.empty());
}
TEST_F(ReadPicoSntpWire, RealSdkReplyAfterCancellationDoesNotApplySystemClock) {
    arm_reply([] { read_pico_sntp_request_stop(); receive(response(sent)); }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(out, 42); EXPECT_EQ(clock_calls, 0);
    EXPECT_EQ(removals, 1); EXPECT_TRUE(timers.empty());
}
TEST_F(ReadPicoSntpWire, RealSdkDuplicateCurrentReplyOnlyAppliesFirstAcceptedTime) {
    arm_reply([] {
        auto bytes = response(sent); receive(bytes);
        uint32_t sec = htonl((uint32_t)1800000010 + UINT32_C(2208988800));
        memcpy(bytes.data() + 40, &sec, 4); receive(bytes);
    }); time_t out = 42;
    EXPECT_EQ(sync(&out), ESP_OK); EXPECT_EQ(clock_calls, 1); EXPECT_EQ(out, 1800000000);
}
