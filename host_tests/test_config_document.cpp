#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdint>
extern "C" {
#include "config_document.h"
}
namespace {
struct Source {
    std::string text = "{}";
    size_t offset = 0, chunk = 1;
    int calls = 0, timeout_calls = 0;
    bool alternating = false, oversized = false;
    esp_err_t failure = ESP_OK;
};
esp_err_t receive(void *context, char *out, size_t capacity, size_t *count) {
    auto *s = static_cast<Source *>(context); ++s->calls;
    if (s->timeout_calls > 0 && (!s->alternating || s->calls % 2)) {
        --s->timeout_calls; return ESP_ERR_TIMEOUT;
    }
    if (s->failure != ESP_OK) return s->failure;
    if (s->oversized) { *count = capacity + 1; return ESP_OK; }
    size_t n = std::min({capacity, s->chunk, s->text.size() - s->offset});
    memcpy(out, s->text.data() + s->offset, n); s->offset += n; *count = n; return ESP_OK;
}
esp_err_t read(Source &s, char *buffer, size_t capacity, size_t length) {
    config_document_source_t ops{&s, receive};
    return config_document_receive(buffer, capacity, length, &ops);
}
esp_err_t parse(const std::string &text, cJSON **out) { return config_document_parse(text.c_str(), text.size(), out); }
int allocations, fail_at, live;
void *allocate(size_t n) { if (++allocations == fail_at) return nullptr; void *p = malloc(n); if (p) ++live; return p; }
void release(void *p) { if (p) --live; free(p); }
}
TEST(ConfigDocument, ReceivesEveryFragmentSizeBeforeParsingOneObject) {
    const std::string text = "{\"toneMode\":\"contrast\",\"exposure\":1}";
    for (size_t chunk = 1; chunk <= text.size(); ++chunk) {
        Source s; s.text = text; s.chunk = chunk; char buffer[128]; memset(buffer, 0x55, sizeof(buffer));
        ASSERT_EQ(read(s, buffer, sizeof(buffer), text.size()), ESP_OK);
        EXPECT_STREQ(buffer, text.c_str()); EXPECT_EQ(buffer[text.size() + 1], 0x55);
        cJSON *root = nullptr; ASSERT_EQ(config_document_parse(buffer, text.size(), &root), ESP_OK);
        EXPECT_EQ(cJSON_GetObjectItem(root, "exposure")->valuedouble, 1); cJSON_Delete(root);
    }
}
TEST(ConfigDocument, RejectsInvalidOrOverflowLengthsBeforeReceive) {
    Source s; char buffer[8]; memset(buffer, 0x55, sizeof(buffer));
    for (size_t n : {size_t(0), size_t(8), size_t(4096), std::numeric_limits<size_t>::max()})
        EXPECT_EQ(read(s, buffer, sizeof(buffer), n), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(s.calls, 0); EXPECT_EQ(buffer[0], 0x55);
}
TEST(ConfigDocument, AcceptsExactMaximumPayloadAndDoesNotOverwriteCanary) {
    Source s; s.text = "{}" + std::string(4093, ' '); s.chunk = 53;
    char buffer[4097]; memset(buffer, 0x55, sizeof(buffer));
    ASSERT_EQ(read(s, buffer, 4096, 4095), ESP_OK); EXPECT_EQ(buffer[4095], 0); EXPECT_EQ(buffer[4096], 0x55);
    cJSON *root = nullptr; ASSERT_EQ(config_document_parse(buffer, 4095, &root), ESP_OK); cJSON_Delete(root);
}
TEST(ConfigDocument, EofAfterValidPrefixStillRefusesDeclaredLongerBody) {
    Source s; char buffer[8]; memset(buffer, 0x55, sizeof(buffer));
    EXPECT_EQ(read(s, buffer, sizeof(buffer), 4), ESP_ERR_INVALID_RESPONSE);
    EXPECT_EQ(s.offset, 2u); EXPECT_EQ(buffer[4], 0x55);
}
TEST(ConfigDocument, IoFailureAndInvalidCallbackCountStopReceive) {
    Source s; char buffer[8]; s.failure = ESP_FAIL;
    EXPECT_EQ(read(s, buffer, sizeof(buffer), 2), ESP_FAIL); EXPECT_EQ(s.calls, 1);
    s.failure = ESP_OK; s.oversized = true;
    EXPECT_EQ(read(s, buffer, sizeof(buffer), 2), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(s.calls, 2);
}
TEST(ConfigDocument, SevenTimeoutsCanCompleteButEighthTimeoutStops) {
    Source s; char buffer[8]; s.timeout_calls = 7;
    EXPECT_EQ(read(s, buffer, sizeof(buffer), 2), ESP_OK);
    Source blocked; blocked.timeout_calls = 8;
    EXPECT_EQ(read(blocked, buffer, sizeof(buffer), 2), ESP_ERR_TIMEOUT);
    EXPECT_EQ(blocked.calls, 8); EXPECT_EQ(blocked.offset, 0u);
}
TEST(ConfigDocument, ProgressDoesNotResetTotalTimeoutBudget) {
    Source s; s.text = std::string(20, ' '); s.alternating = true; s.timeout_calls = 8; char buffer[32];
    EXPECT_EQ(read(s, buffer, sizeof(buffer), 20), ESP_ERR_TIMEOUT);
    EXPECT_EQ(s.calls, 15); EXPECT_EQ(s.offset, 7u);
}
TEST(ConfigDocument, InvalidReceiveArgumentsDoNotInvokeSource) {
    Source s; config_document_source_t invalid{&s, nullptr}; char buffer[8];
    EXPECT_EQ(config_document_receive(buffer, sizeof(buffer), 2, &invalid), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_document_receive(buffer, sizeof(buffer), 2, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read(s, nullptr, sizeof(buffer), 2), ESP_ERR_INVALID_ARG); EXPECT_EQ(s.calls, 0);
}
TEST(ConfigDocument, ParsesUnknownFieldsWhitespaceAndEscapedBracesWithoutChangingSemantics) {
    cJSON *root = nullptr; ASSERT_EQ(parse(" \n{\"x\":\"\\\\u0000 {[] }\",\"other\":[1,true]} \t", &root), ESP_OK);
    EXPECT_STREQ(cJSON_GetObjectItem(root, "x")->valuestring, "\\u0000 {[] }"); cJSON_Delete(root);
}
TEST(ConfigDocument, RejectsWrongRootTrailingDocumentsDuplicatesAndDecodedNul) {
    for (auto text : {"[]", "null", "true", "1", "{}suffix", "{}{}", "{\"x\":1,\"X\":2}",
                      "{\"x\":[{\"a\":1,\"A\":2}]}", "{\"x\\u0000hidden\":1}",
                      "{\"x\":\"a\\u0000b\"}", "{\"x\":1e999}"}) {
        cJSON *sentinel = reinterpret_cast<cJSON *>(uintptr_t(1));
        EXPECT_EQ(parse(text, &sentinel), ESP_ERR_INVALID_RESPONSE) << text;
        EXPECT_EQ(sentinel, reinterpret_cast<cJSON *>(uintptr_t(1)));
    }
}
TEST(ConfigDocument, RawNulOrMissingFinalTerminatorCannotHideSuffix) {
    const char raw[] = {'{', '}', 0, 'x', 0}; cJSON *root = nullptr;
    EXPECT_EQ(config_document_parse(raw, 4, &root), ESP_ERR_INVALID_RESPONSE);
    const char text[] = "{}suffix";
    EXPECT_EQ(config_document_parse(text, 2, &root), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(root, nullptr);
}
TEST(ConfigDocument, DepthSixteenAcceptedAndSeventeenRejectedBeforeAllocation) {
    std::string text = "{\"x\":" + std::string(15, '[') + "0" + std::string(15, ']') + "}";
    cJSON *root = nullptr; ASSERT_EQ(parse(text, &root), ESP_OK); cJSON_Delete(root); root = nullptr;
    text = "{\"x\":" + std::string(16, '[') + "0" + std::string(16, ']') + "}";
    EXPECT_EQ(parse(text, &root), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(root, nullptr);
}
TEST(ConfigDocument, EveryParserAllocationFailureLeavesOutputUntouchedAndNoLeaks) {
    allocations = live = 0; fail_at = -1; cJSON_Hooks hooks{allocate, release}; cJSON_InitHooks(&hooks);
    std::string text = "{\"toneMode\":\"contrast\",\"values\":[1,2]}";
    cJSON *root = nullptr; EXPECT_EQ(parse(text, &root), ESP_OK); int total = allocations; cJSON_Delete(root);
    for (int i = 1; i <= total; ++i) {
        allocations = 0; fail_at = i; root = reinterpret_cast<cJSON *>(uintptr_t(1));
        EXPECT_EQ(parse(text, &root), ESP_ERR_INVALID_RESPONSE) << i;
        EXPECT_EQ(root, reinterpret_cast<cJSON *>(uintptr_t(1))); EXPECT_EQ(live, 0);
    }
    cJSON_InitHooks(nullptr);
}
TEST(ConfigDocument, InvalidParserArgumentsAndCapacityLeaveOutputUntouched) {
    cJSON *root = nullptr;
    EXPECT_EQ(config_document_parse(nullptr, 2, &root), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_document_parse("{}", 2, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_document_parse("", 0, &root), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_document_parse("{}", 4096, &root), ESP_ERR_INVALID_SIZE); EXPECT_EQ(root, nullptr);
}
