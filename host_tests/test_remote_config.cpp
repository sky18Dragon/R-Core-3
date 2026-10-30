#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "remote_config.h"
#include "config_apply.h"
}
namespace {
std::vector<remote_config_stage_t> calls;
remote_config_stage_t failure;
int finish_calls, allocation_count, allocation_fail_at, live_allocations;
void *allocate(size_t n) {
    if (++allocation_count == allocation_fail_at) return nullptr;
    void *p = malloc(n); if (p) ++live_allocations; return p;
}
void release(void *p) { if (p) --live_allocations; free(p); }
esp_err_t apply(void *, remote_config_stage_t stage, cJSON *group) {
    EXPECT_TRUE(cJSON_IsObject(group)); calls.push_back(stage);
    return stage == failure ? ESP_ERR_NOT_FINISHED : ESP_OK;
}
esp_err_t finish(void *) { ++finish_calls; return failure == REMOTE_CONFIG_REVISION ? ESP_FAIL : ESP_OK; }
remote_config_ops_t ops{nullptr, apply, finish};
class RemoteConfig : public testing::Test {
protected:
    void SetUp() override {
        calls.clear(); failure = REMOTE_CONFIG_NONE; finish_calls = allocation_count = live_allocations = 0;
        allocation_fail_at = -1; cJSON_Hooks hooks{allocate, release}; cJSON_InitHooks(&hooks);
    }
    void TearDown() override { EXPECT_EQ(live_allocations, 0); cJSON_InitHooks(nullptr); }
};
}
TEST_F(RemoteConfig, AllGroupsRunInUpstreamOrderThenOneCheckedFinish) {
    remote_config_result_t result{};
    ASSERT_EQ(remote_config_apply("{\"color_palette\":{\"gamma\":1},\"processing_settings\":{\"exposure\":0},\"config\":{\"timezone\":\"UTC0\"}}", &ops, &result), ESP_OK);
    EXPECT_EQ(calls, (std::vector<remote_config_stage_t>{REMOTE_CONFIG_GENERAL, REMOTE_CONFIG_PROCESSING, REMOTE_CONFIG_PALETTE}));
    EXPECT_EQ(finish_calls, 1); EXPECT_EQ(result.completed, 7u); EXPECT_TRUE(result.revision_confirmed);
    EXPECT_EQ(result.failed_stage, REMOTE_CONFIG_NONE);
}

namespace {
struct GuardedRemote { const char *payload; remote_config_result_t result{}; int blocked = 0; };
esp_err_t guarded_group(void *context, remote_config_stage_t stage, cJSON *group) {
    auto *request = static_cast<GuardedRemote *>(context);
    EXPECT_EQ(config_apply_run([](void *p) -> esp_err_t { ++*static_cast<int *>(p); return ESP_OK; },
                               &request->blocked), ESP_ERR_INVALID_STATE);
    return apply(nullptr, stage, group);
}
esp_err_t guarded_finish(void *context) {
    auto *request = static_cast<GuardedRemote *>(context);
    EXPECT_EQ(config_apply_run([](void *p) -> esp_err_t { ++*static_cast<int *>(p); return ESP_OK; },
                               &request->blocked), ESP_ERR_INVALID_STATE);
    return finish(nullptr);
}
esp_err_t guarded_remote(void *context) {
    auto *request = static_cast<GuardedRemote *>(context);
    remote_config_ops_t operations{context, guarded_group, guarded_finish};
    return remote_config_apply(request->payload, &operations, &request->result);
}
}
TEST_F(RemoteConfig, ApplicationControlCoversAllParsedGroupsAndRevision) {
    GuardedRemote request{"{\"config\":{\"x\":1},\"processing_settings\":{\"x\":1},\"color_palette\":{\"x\":1}}"};
    EXPECT_EQ(config_apply_run(guarded_remote, &request), ESP_OK);
    EXPECT_EQ(calls.size(), 3u); EXPECT_EQ(finish_calls, 1); EXPECT_EQ(request.blocked, 0);
    EXPECT_TRUE(request.result.revision_confirmed);
}
TEST_F(RemoteConfig, FailedGroupAndRevisionBothReleaseWholeApplicationControl) {
    GuardedRemote request{"{\"config\":{\"x\":1},\"color_palette\":{\"x\":1}}"};
    for (auto stage : {REMOTE_CONFIG_GENERAL, REMOTE_CONFIG_PALETTE, REMOTE_CONFIG_REVISION}) {
        failure = stage;
        EXPECT_NE(config_apply_run(guarded_remote, &request), ESP_OK);
        EXPECT_FALSE(request.result.revision_confirmed);
        failure = REMOTE_CONFIG_NONE;
        EXPECT_EQ(config_apply_run(guarded_remote, &request), ESP_OK);
        EXPECT_TRUE(request.result.revision_confirmed);
    }
    EXPECT_EQ(request.blocked, 0);
}
TEST_F(RemoteConfig, FailureInEachGroupStopsLaterCallbacksAndNeverConfirmsRevision) {
    for (unsigned stage = 0; stage < 3; ++stage) {
        calls.clear(); failure = (remote_config_stage_t)stage; remote_config_result_t result{};
        EXPECT_EQ(remote_config_apply("{\"config\":{\"timezone\":\"UTC0\"},\"processing_settings\":{\"exposure\":0},\"color_palette\":{\"gamma\":1}}", &ops, &result), ESP_ERR_NOT_FINISHED);
        EXPECT_EQ(calls.size(), stage + 1); EXPECT_EQ(result.completed, (1u << stage) - 1);
        EXPECT_EQ(result.failed_stage, failure); EXPECT_FALSE(result.revision_confirmed); EXPECT_EQ(finish_calls, 0);
    }
}
TEST_F(RemoteConfig, FinishFailureRetainsAppliedStageMaskButDoesNotReportComplete) {
    failure = REMOTE_CONFIG_REVISION; remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply("{\"config\":{\"timezone\":\"UTC0\"}}", &ops, &result), ESP_FAIL);
    EXPECT_EQ(result.completed, 1u); EXPECT_EQ(result.failed_stage, REMOTE_CONFIG_REVISION); EXPECT_FALSE(result.revision_confirmed);
}
TEST_F(RemoteConfig, MissingEmptyAndUnknownRootMetadataDoNotTouchRevision) {
    for (auto json : {"{}", "{\"config\":{},\"color_palette\":{}}", "{\"server_version\":2}"}) {
        remote_config_result_t result{}; EXPECT_EQ(remote_config_apply(json, &ops, &result), ESP_OK);
        EXPECT_EQ(result.completed, 0u); EXPECT_FALSE(result.revision_confirmed);
    }
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(finish_calls, 0);
}
TEST_F(RemoteConfig, UnknownRootMetadataAlongsideRecognizedGroupIsIgnoredLikeUpstream) {
    remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply("{\"server_version\":2,\"CONFIG\":{\"timezone\":\"UTC0\"}}", &ops, &result), ESP_OK);
    EXPECT_EQ(result.completed, 1u); EXPECT_EQ(finish_calls, 1);
}
TEST_F(RemoteConfig, WrongRootOrKnownGroupTypesRejectBeforeAnyApply) {
    for (auto json : {"null", "[]", "1", "\"string\"", "{\"config\":null}", "{\"processing_settings\":[]}",
                     "{\"config\":{\"timezone\":\"UTC0\"},\"color_palette\":1}"}) {
        remote_config_result_t result{}; EXPECT_EQ(remote_config_apply(json, &ops, &result), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(result.failed_stage, REMOTE_CONFIG_PARSE);
    }
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(finish_calls, 0);
}
TEST_F(RemoteConfig, InvalidTruncatedTrailingOrMultipleJsonDocumentsReject) {
    for (auto json : {"", " ", "{", "{\"config\":{", "{}suffix", "{}{}", "{]", "{\"config\":{\"x\":}}"}) {
        remote_config_result_t result{}; EXPECT_EQ(remote_config_apply(json, &ops, &result), ESP_ERR_INVALID_RESPONSE);
    }
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(finish_calls, 0);
}
TEST_F(RemoteConfig, TrailingWhitespaceAcceptedWithoutExtraDocument) {
    remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply(" {\"config\":{\"timezone\":\"UTC0\"}} \n\t", &ops, &result), ESP_OK);
    EXPECT_TRUE(result.revision_confirmed);
}
TEST_F(RemoteConfig, RootNestedAndCaseInsensitiveDuplicateKeysRejectBeforeApply) {
    for (auto json : {"{\"config\":{},\"config\":{}}", "{\"config\":{},\"CONFIG\":{}}",
                     "{\"config\":{\"timezone\":\"UTC0\",\"TIMEZONE\":\"CST-8\"}}",
                     "{\"config\":{\"a\":[{\"x\":1,\"X\":2}]}}"}) {
        remote_config_result_t result{}; EXPECT_EQ(remote_config_apply(json, &ops, &result), ESP_ERR_INVALID_RESPONSE);
    }
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(finish_calls, 0);
}
TEST_F(RemoteConfig, DecodedNulInKeyOrValueCannotHideSuffix) {
    for (auto json : {"{\"config\\u0000suffix\":{}}", "{\"config\":{\"timezone\":\"UTC0\\u0000suffix\"}}"}) {
        remote_config_result_t result{}; EXPECT_EQ(remote_config_apply(json, &ops, &result), ESP_ERR_INVALID_RESPONSE);
    }
    EXPECT_TRUE(calls.empty()); EXPECT_EQ(allocation_count, 0);
}
TEST_F(RemoteConfig, LiteralEscapedBackslashAndBracesInStringsDoNotTriggerNulOrDepthGuard) {
    remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply("{\"config\":{\"x\":\"\\\\u0000 [{{}}] \\\"quote\\\"\"}}", &ops, &result), ESP_OK);
    EXPECT_TRUE(result.revision_confirmed);
}
TEST_F(RemoteConfig, NonfiniteParsedNumbersNeverReachBusinessConverters) {
    remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply("{\"processing_settings\":{\"exposure\":1e999}}", &ops, &result), ESP_ERR_INVALID_RESPONSE);
    EXPECT_TRUE(calls.empty());
}
TEST_F(RemoteConfig, OverlongPayloadRejectedBeforeAllocationOrApply) {
    std::string value(REMOTE_CONFIG_CAPACITY, ' '); remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply(value.c_str(), &ops, &result), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(allocation_count, 0); EXPECT_TRUE(calls.empty());
}
TEST_F(RemoteConfig, MaximumLengthPayloadAcceptedWithoutTruncation) {
    std::string value = "{}" + std::string(REMOTE_CONFIG_CAPACITY - 3, ' '); remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply(value.c_str(), &ops, &result), ESP_OK); EXPECT_EQ(finish_calls, 0);
}
TEST_F(RemoteConfig, ExcessiveNestingRejectedBeforeParserAllocation) {
    std::string value(17, '['); value += "0" + std::string(17, ']'); remote_config_result_t result{};
    EXPECT_EQ(remote_config_apply(value.c_str(), &ops, &result), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(allocation_count, 0);
}
TEST_F(RemoteConfig, ParserAllocationFailureDoesNotApplyOrLeak) {
    remote_config_result_t baseline{};
    ASSERT_EQ(remote_config_apply("{\"config\":{\"timezone\":\"UTC0\"}}", &ops, &baseline), ESP_OK);
    int parser_allocations = allocation_count;
    finish_calls = 0;
    for (int i = 1; i <= parser_allocations; ++i) {
        calls.clear(); allocation_count = 0; allocation_fail_at = i; remote_config_result_t result{};
        EXPECT_EQ(remote_config_apply("{\"config\":{\"timezone\":\"UTC0\"}}", &ops, &result), ESP_ERR_INVALID_RESPONSE);
        EXPECT_TRUE(calls.empty()); EXPECT_EQ(live_allocations, 0); EXPECT_EQ(finish_calls, 0);
    }
}
TEST_F(RemoteConfig, InvalidArgumentsPreserveResultAndDoNotAllocate) {
    remote_config_result_t result{99, REMOTE_CONFIG_NONE, true}; auto invalid = ops;
    EXPECT_EQ(remote_config_apply(nullptr, &ops, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(remote_config_apply("{}", nullptr, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(remote_config_apply("{}", &ops, nullptr), ESP_ERR_INVALID_ARG);
    invalid.apply = nullptr; EXPECT_EQ(remote_config_apply("{}", &invalid, &result), ESP_ERR_INVALID_ARG);
    invalid = ops; invalid.finish = nullptr; EXPECT_EQ(remote_config_apply("{}", &invalid, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(result.completed, 99u); EXPECT_TRUE(result.revision_confirmed); EXPECT_EQ(allocation_count, 0);
}
TEST_F(RemoteConfig, HeaderExactCapacityAcceptedOverlongRejectedWithoutChangingTarget) {
    char out[8] = "old"; bool seen = false;
    EXPECT_EQ(remote_config_capture(out, sizeof(out), &seen, "1234567"), ESP_OK); EXPECT_STREQ(out, "1234567");
    seen = false; EXPECT_EQ(remote_config_capture(out, sizeof(out), &seen, "12345678"), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(out, "1234567"); EXPECT_TRUE(seen);
}
TEST_F(RemoteConfig, DuplicateHeaderEvenAfterEmptyFirstValueIsNotLastWriterWins) {
    char out[16] = "old"; bool seen = false;
    EXPECT_EQ(remote_config_capture(out, sizeof(out), &seen, ""), ESP_OK);
    EXPECT_EQ(remote_config_capture(out, sizeof(out), &seen, "{}"), ESP_ERR_INVALID_RESPONSE); EXPECT_STREQ(out, "");
    seen = false; EXPECT_EQ(remote_config_capture(out, sizeof(out), &seen, "{}"), ESP_OK); EXPECT_STREQ(out, "{}");
}
TEST_F(RemoteConfig, HeaderInvalidArgumentsLeaveOutputAndSeenUnchanged) {
    char out[8] = "old"; bool seen = false;
    EXPECT_EQ(remote_config_capture(nullptr, 8, &seen, "x"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(remote_config_capture(out, 8, nullptr, "x"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(remote_config_capture(out, 8, &seen, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(remote_config_capture(out, 0, &seen, "x"), ESP_ERR_INVALID_ARG);
    EXPECT_FALSE(seen); EXPECT_STREQ(out, "old");
}
