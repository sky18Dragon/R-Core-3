#include <gtest/gtest.h>
#include <string>
#include <cstring>
extern "C" {
#include "provisioning_apply.h"
}
namespace {
struct Context {
    std::string calls;
    int test_error = 0, save_error = 0, response_error = 0;
    int current_calls = 0, expire_at = 0;
    bool complete = true;
    provisioning_apply_request_t *mutate = nullptr;
    provisioning_apply_request_t tested{}, saved{};
};
bool current(void *p, uint32_t ticket) {
    auto &c = *(Context *)p; c.calls += 'C'; EXPECT_EQ(ticket, 3u);
    return ++c.current_calls != c.expire_at;
}
int test(void *p, const provisioning_apply_request_t *request) {
    auto &c = *(Context *)p; c.calls += 'T'; c.tested = *request;
    if (c.mutate) memset(c.mutate, 0, sizeof(*c.mutate));
    return c.test_error;
}
int save(void *p, const provisioning_apply_request_t *request) {
    auto &c = *(Context *)p; c.calls += 'S'; c.saved = *request; return c.save_error;
}
int response(void *p, const provisioning_apply_request_t *) {
    auto &c = *(Context *)p; c.calls += 'R'; return c.response_error;
}
bool complete(void *p, uint32_t ticket) {
    auto &c = *(Context *)p; c.calls += 'F'; EXPECT_EQ(ticket, 3u); return c.complete;
}
const provisioning_apply_ops_t ops{current, test, save, response, complete};
provisioning_apply_request_t request() {
    provisioning_apply_request_t r{}; r.ticket = 3;
    strcpy(r.ssid, "network"); strcpy(r.password, "password"); strcpy(r.name, "name");
    EXPECT_TRUE(wifi_network_copy(&r.network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    return r;
}
}
TEST(ProvisioningApply, TestSaveResponseAndCurrentCompletionAreStrictlyOrdered) {
    auto r = request(); Context c;
    auto result = provisioning_apply_run(&r, &ops, &c);
    EXPECT_EQ(result.stage, PROVISION_APPLY_DONE); EXPECT_EQ(result.error, 0);
    EXPECT_EQ(c.calls, "CTCSCRF");
    EXPECT_EQ(c.tested.ticket, c.saved.ticket); EXPECT_STREQ(c.tested.ssid, c.saved.ssid);
    EXPECT_STREQ(c.tested.password, c.saved.password); EXPECT_STREQ(c.tested.name, c.saved.name);
    EXPECT_STREQ(c.tested.network.dns, c.saved.network.dns);
}
TEST(ProvisioningApply, InvalidRequestOrMissingCallbacksHasNoSideEffects) {
    auto r = request(); Context c;
    EXPECT_EQ(provisioning_apply_run(nullptr, &ops, &c).stage, PROVISION_APPLY_INVALID);
    EXPECT_EQ(provisioning_apply_run(&r, nullptr, &c).stage, PROVISION_APPLY_INVALID);
    auto missing = ops; missing.test = nullptr;
    EXPECT_EQ(provisioning_apply_run(&r, &missing, &c).stage, PROVISION_APPLY_INVALID);
    r.ticket = 0; EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_INVALID);
    r = request(); memset(r.ssid, 's', sizeof(r.ssid));
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_INVALID);
    r = request(); memset(r.name, 'n', sizeof(r.name));
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_INVALID);
    r = request(); r.network.ip[0] = 'x';
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_INVALID);
    EXPECT_TRUE(c.calls.empty());
}
TEST(ProvisioningApply, StaleInitialSessionDoesNotTestOrSave) {
    auto r = request(); Context c; c.expire_at = 1;
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_STALE);
    EXPECT_EQ(c.calls, "C");
}
TEST(ProvisioningApply, ConnectionFailureCannotSaveRespondOrComplete) {
    auto r = request(); Context c; c.test_error = 123;
    auto result = provisioning_apply_run(&r, &ops, &c);
    EXPECT_EQ(result.stage, PROVISION_APPLY_TEST); EXPECT_EQ(result.error, 123); EXPECT_EQ(c.calls, "CT");
}
TEST(ProvisioningApply, SessionExpiredDuringTestCannotSave) {
    auto r = request(); Context c; c.expire_at = 2;
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_STALE);
    EXPECT_EQ(c.calls, "CTC");
}
TEST(ProvisioningApply, SaveFailureCannotSendSuccessOrComplete) {
    auto r = request(); Context c; c.save_error = 456;
    auto result = provisioning_apply_run(&r, &ops, &c);
    EXPECT_EQ(result.stage, PROVISION_APPLY_SAVE); EXPECT_EQ(result.error, 456); EXPECT_EQ(c.calls, "CTCS");
}
TEST(ProvisioningApply, ResponseFailureDoesNotCompleteOrRetryAlreadySavedGroup) {
    auto r = request(); Context c; c.response_error = 789;
    auto result = provisioning_apply_run(&r, &ops, &c);
    EXPECT_EQ(result.stage, PROVISION_APPLY_RESPONSE); EXPECT_EQ(result.error, 789); EXPECT_EQ(c.calls, "CTCSCR");
}
TEST(ProvisioningApply, FailedCompletionIsNotSuccessOrRollback) {
    auto r = request(); Context c; c.complete = false;
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_COMPLETE);
    EXPECT_EQ(c.calls, "CTCSCRF"); EXPECT_STREQ(c.saved.ssid, "network");
}
TEST(ProvisioningApply, OriginalRequestMutationCannotChangeSavedCandidateOrTicket) {
    auto r = request(); Context c; c.mutate = &r;
    ASSERT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_DONE);
    EXPECT_EQ(c.calls, "CTCSCRF"); EXPECT_EQ(r.ticket, 0u);
    EXPECT_EQ(c.saved.ticket, 3u); EXPECT_STREQ(c.saved.network.dns, "8.8.8.8");
    EXPECT_STREQ(c.tested.password, c.saved.password); EXPECT_STREQ(c.tested.network.ip, c.saved.network.ip);
}
TEST(ProvisioningApply, ExpirationDuringSaveReportsSavedStaleWithoutSuccessResponseOrCompletion) {
    auto r = request(); Context c; c.expire_at = 3;
    EXPECT_EQ(provisioning_apply_run(&r, &ops, &c).stage, PROVISION_APPLY_SAVED_STALE);
    EXPECT_EQ(c.calls, "CTCSC"); EXPECT_STREQ(c.saved.ssid, "network");
}
