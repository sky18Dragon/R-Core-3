#include <gtest/gtest.h>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <limits>
#include <set>
extern "C" {
#include "processing_settings.h"
#include "color_palette.h"
#include "nvs.h"
#include "config_apply.h"
#include "request_config.h"
#if READ_PICO_REQUEST_HEADER_SDK_TESTS
#include "http_header.h"
#endif
}
namespace {
struct RedirectSequence {
    std::vector<int> calls;
    int fail = 0;
    static esp_err_t step(void *context, int stage) {
        auto &self = *static_cast<RedirectSequence *>(context);
        self.calls.push_back(stage);
        return stage == self.fail ? ESP_ERR_NO_MEM : ESP_OK;
    }
    static esp_err_t prepare(void *c) { return step(c, 1); }
    static esp_err_t host(void *c) { return step(c, 2); }
    static esp_err_t schedule(void *c) { return step(c, 3); }
};
}
TEST(RequestRedirect, EveryPreparationFailureStopsBeforeScheduling) {
    for (int failure = 0; failure <= 3; ++failure) {
        RedirectSequence state; state.fail = failure;
        request_redirect_ops_t ops{&state, RedirectSequence::prepare, RedirectSequence::host, RedirectSequence::schedule};
        EXPECT_EQ(request_redirect_schedule_checked(&ops), failure ? ESP_ERR_NO_MEM : ESP_OK);
        std::vector<int> expected{1};
        if (failure != 1) expected.push_back(2);
        if (!failure || failure == 3) expected.push_back(3);
        EXPECT_EQ(state.calls, expected);
    }
}
TEST(RequestRedirect, MissingCallbacksNeverStartPreparation) {
    RedirectSequence state;
    request_redirect_ops_t full{&state, RedirectSequence::prepare, RedirectSequence::host, RedirectSequence::schedule};
    EXPECT_EQ(request_redirect_schedule_checked(nullptr), ESP_ERR_INVALID_ARG);
    for (int missing = 0; missing < 3; ++missing) {
        auto ops = full;
        if (missing == 0) ops.prepare_url = nullptr;
        if (missing == 1) ops.confirm_host = nullptr;
        if (missing == 2) ops.schedule = nullptr;
        EXPECT_EQ(request_redirect_schedule_checked(&ops), ESP_ERR_INVALID_ARG);
        EXPECT_TRUE(state.calls.empty());
    }
}
namespace {
std::map<std::string, std::string> saved;
std::vector<std::string> keys;
int opens, sets, commits, closes, active, fail_set, erases;
esp_err_t erase_error;
esp_err_t open_error, set_error, commit_error, blob_error;
std::map<std::string, esp_err_t> get_errors;
int read_calls, json_allocations, json_fail_at, json_live;
bool short_blob;
bool probe_options_control;
int attempted_writers;
int revision_calls;
int64_t test_revision;
esp_err_t revision_error;
#if READ_PICO_REQUEST_HEADER_SDK_TESTS
std::set<void *> sdk_header_live;
int sdk_header_allocations = 0, sdk_header_fail_at = -1;
#endif
esp_err_t attempted_write(void *) { ++attempted_writers; return ESP_OK; }
void *json_allocate(size_t n) {
    if (++json_allocations == json_fail_at) return nullptr;
    void *p = malloc(n); if (p) ++json_live; return p;
}
void json_release(void *p) { if (p) --json_live; free(p); }
const char *expected_keys[] = {"proc_exp", "proc_sat", "proc_tone", "proc_cont", "proc_str",
                             "proc_shad", "proc_high", "proc_mid", "proc_col", "proc_cdr",
                             "proc_dith", "proc_smode", "proc_bgcol"};
esp_err_t write(const char *key, const void *value, size_t n) {
    EXPECT_EQ(active, 1); keys.emplace_back(key); ++sets;
    if (sets == fail_set) return set_error;
    saved[key] = std::string((const char *)value, n); return ESP_OK;
}
class ReadPicoSettingsSave : public testing::Test {
protected:
    void SetUp() override {
        saved.clear(); keys.clear(); get_errors.clear(); read_calls = json_allocations = json_live = 0; json_fail_at = -1;
        opens = sets = commits = closes = active = 0; fail_set = -1;
        probe_options_control = false; attempted_writers = 0;
        revision_calls = 0; test_revision = 1234567890123; revision_error = ESP_OK;
#if READ_PICO_REQUEST_HEADER_SDK_TESTS
        EXPECT_TRUE(sdk_header_live.empty()); sdk_header_allocations = 0; sdk_header_fail_at = -1;
#endif
        open_error = commit_error = blob_error = ESP_OK; set_error = ESP_ERR_NO_MEM; short_blob = false;
        erases = 0; erase_error = ESP_OK;
    }
    void TearDown() override { EXPECT_EQ(active, 0); EXPECT_EQ(json_live, 0); cJSON_InitHooks(nullptr); }
};
esp_err_t read_value(const char *key, void *out, size_t n) {
    ++read_calls;
    if (probe_options_control) EXPECT_EQ(config_apply_run(attempted_write, nullptr), ESP_ERR_INVALID_STATE);
    if (get_errors.count(key)) { memset(out, 0xaa, n); return get_errors[key]; }
    auto it = saved.find(key); if (it == saved.end()) return ESP_ERR_NVS_NOT_FOUND;
    if (it->second.size() != n) return ESP_ERR_NVS_TYPE_MISMATCH;
    memcpy(out, it->second.data(), n); return ESP_OK;
}
TEST_F(ReadPicoSettingsSave, OptionsMissingNamespaceOrKeysRetainUpstreamDefaults) {
    for (esp_err_t missing : {ESP_OK, ESP_ERR_NVS_NOT_FOUND}) {
        open_error = missing;
        processing_options_t out{};
        ASSERT_EQ(processing_settings_copy_options(&out), ESP_OK);
        EXPECT_EQ(out.dither_algorithm, DITHER_FLOYD_STEINBERG);
        EXPECT_EQ(out.scale_mode, SCALE_MODE_COVER);
        EXPECT_STREQ(out.background_color, "white");
    }
    EXPECT_EQ(sets, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ReadPicoSettingsSave, OptionsMapAllSupportedAlgorithmsLayoutsAndBackgroundsWithOneRead) {
    const char *algorithms[] = {"floyd-steinberg", "stucki", "burkes", "sierra"};
    for (int a = 0; a < 4; ++a) for (int fit = 0; fit < 2; ++fit) for (const char *bg : {"white", "black"}) {
        saved["proc_dith"] = std::string(algorithms[a]) + '\0';
        saved["proc_smode"] = std::string(fit ? "fit" : "cover") + '\0';
        saved["proc_bgcol"] = std::string(bg) + '\0';
        opens = read_calls = closes = 0;
        processing_options_t out{};
        ASSERT_EQ(processing_settings_copy_options(&out), ESP_OK);
        EXPECT_EQ(out.dither_algorithm, a); EXPECT_EQ(out.scale_mode, fit);
        EXPECT_STREQ(out.background_color, bg);
        EXPECT_EQ(opens, 1); EXPECT_EQ(read_calls, 13); EXPECT_EQ(closes, 1);
    }
}
TEST_F(ReadPicoSettingsSave, OptionsEveryReadErrorPreservesOutputAndReleasesControlForRetry) {
    for (const char *key : expected_keys) {
        processing_options_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
        get_errors[key] = ESP_ERR_NO_MEM;
        EXPECT_EQ(processing_settings_copy_options(&out), ESP_ERR_NO_MEM) << key;
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(active, 0);
        get_errors.clear();
        EXPECT_EQ(processing_settings_copy_options(&out), ESP_OK);
    }
    EXPECT_EQ(sets, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ReadPicoSettingsSave, OptionsRejectOpenFailureWithoutDefaultPublication) {
    processing_options_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
    open_error = ESP_FAIL;
    EXPECT_EQ(processing_settings_copy_options(&out), ESP_FAIL);
    EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(read_calls, 0); EXPECT_EQ(closes, 0);
    open_error = ESP_OK; EXPECT_EQ(processing_settings_copy_options(&out), ESP_OK);
}
TEST_F(ReadPicoSettingsSave, OptionsRejectUnsupportedStoredConsumedEnumsWithoutPublication) {
    for (const char *key : {"proc_dith", "proc_smode", "proc_bgcol"}) for (const char *bad : {"", "unknown", "WHITE"}) {
        saved.clear(); saved[key] = std::string(bad) + '\0';
        processing_options_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
        EXPECT_EQ(processing_settings_copy_options(&out), ESP_ERR_INVALID_ARG);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(active, 0);
    }
}
TEST_F(ReadPicoSettingsSave, OptionsBusyDoesNotReadOrPublishAndNullDoesNotEnterControl) {
    processing_options_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
    auto busy = [](void *ctx) -> esp_err_t {
        EXPECT_EQ(processing_settings_copy_options((processing_options_t *)ctx), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(processing_settings_copy_options(nullptr), ESP_ERR_INVALID_ARG);
        return ESP_OK;
    };
    EXPECT_EQ(config_apply_run(busy, &out), ESP_OK);
    EXPECT_EQ(opens, 0); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    EXPECT_EQ(processing_settings_copy_options(&out), ESP_OK);
}
TEST_F(ReadPicoSettingsSave, OptionsReaderExcludesParticipatingApplicationUntilCopyCompletes) {
    probe_options_control = true;
    processing_options_t out{};
    ASSERT_EQ(processing_settings_copy_options(&out), ESP_OK);
    EXPECT_EQ(read_calls, 13); EXPECT_EQ(attempted_writers, 0);
    EXPECT_EQ(config_apply_run(attempted_write, nullptr), ESP_OK); EXPECT_EQ(attempted_writers, 1);
}
TEST_F(ReadPicoSettingsSave, OptionsOutputIsOwnedAndDoesNotChangeAfterLaterSettingsRead) {
    saved["proc_bgcol"] = std::string("black") + '\0';
    processing_options_t first{}, second{};
    ASSERT_EQ(processing_settings_copy_options(&first), ESP_OK);
    saved["proc_bgcol"] = std::string("white") + '\0';
    ASSERT_EQ(processing_settings_copy_options(&second), ESP_OK);
    EXPECT_STREQ(first.background_color, "black"); EXPECT_STREQ(second.background_color, "white");
}
}
extern "C" {
int test_board_display_width = 1216, test_board_display_height = 684;
esp_err_t config_manager_copy_config_last_updated(int64_t *out) {
    ++revision_calls;
    if (probe_options_control) EXPECT_EQ(config_apply_run(attempted_write, nullptr), ESP_ERR_INVALID_STATE);
    if (revision_error != ESP_OK) return revision_error;
    *out = test_revision; return ESP_OK;
}
const char *test_board_display_type = "gc16";
const char *esp_err_to_name(esp_err_t) { return "synthetic error"; }
esp_err_t nvs_open(const char *, int, nvs_handle_t *out) {
    ++opens; if (open_error != ESP_OK) return open_error; ++active; *out = 1; return ESP_OK;
}
void nvs_close(nvs_handle_t h) { EXPECT_EQ(h, 1); --active; ++closes; }
esp_err_t nvs_set_u32(nvs_handle_t, const char *key, uint32_t value) { return write(key, &value, sizeof(value)); }
esp_err_t nvs_set_u8(nvs_handle_t, const char *key, uint8_t value) { return write(key, &value, sizeof(value)); }
esp_err_t nvs_set_str(nvs_handle_t, const char *key, const char *value) { return write(key, value, strlen(value) + 1); }
esp_err_t nvs_set_blob(nvs_handle_t, const char *key, const void *value, size_t n) { return write(key, value, n); }
esp_err_t nvs_erase_key(nvs_handle_t, const char *key) {
    ++erases; if (erase_error) return erase_error;
    return saved.erase(key) ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}
esp_err_t nvs_commit(nvs_handle_t h) { EXPECT_EQ(h, 1); ++commits; return commit_error; }
esp_err_t nvs_get_u32(nvs_handle_t, const char *key, uint32_t *out) { return read_value(key, out, sizeof(*out)); }
esp_err_t nvs_get_u8(nvs_handle_t, const char *key, uint8_t *out) { return read_value(key, out, sizeof(*out)); }
esp_err_t nvs_get_str(nvs_handle_t, const char *key, char *out, size_t *n) {
    ++read_calls;
    if (get_errors.count(key)) { memset(out, 0xaa, *n); return get_errors[key]; }
    auto it = saved.find(key); if (it == saved.end()) return ESP_ERR_NVS_NOT_FOUND;
    size_t actual = it->second.size();
    if (actual > *n) { *n = actual; return ESP_ERR_NVS_INVALID_LENGTH; }
    memcpy(out, it->second.data(), actual); *n = actual; return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t, const char *key, void *value, size_t *n) {
    if (blob_error) { memset(value, 0xaa, *n); return blob_error; }
    auto found = saved.find(key); if (found == saved.end()) return ESP_ERR_NVS_NOT_FOUND;
    size_t size = short_blob ? found->second.size() - 1 : found->second.size();
    if (size > *n) return ESP_ERR_NVS_INVALID_LENGTH;
    memcpy(value, found->second.data(), size); *n = size; return ESP_OK;
}
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotCapturesOwnedCompleteJsonAndRevisionWithNoWrites) {
    request_config_t out{};
    ASSERT_EQ(request_config_capture(&out), ESP_OK);
    EXPECT_STREQ(out.revision, "1234567890123"); EXPECT_EQ(revision_calls, 1); EXPECT_EQ(opens, 2);
    auto processing = cJSON_Parse(out.processing), palette = cJSON_Parse(out.palette);
    ASSERT_NE(processing, nullptr); ASSERT_NE(palette, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(processing), 13); EXPECT_EQ(cJSON_GetArraySize(palette), 3);
    EXPECT_STREQ(cJSON_GetObjectItem(processing, "scaleMode")->valuestring, "cover");
    EXPECT_NEAR(cJSON_GetObjectItem(palette, "gamma")->valuedouble, 1.42, 1e-6);
    cJSON_Delete(processing); cJSON_Delete(palette);
    EXPECT_EQ(sets, 0); EXPECT_EQ(commits, 0);
    request_config_release(&out); EXPECT_EQ(out.processing, nullptr); EXPECT_EQ(out.palette, nullptr);
    EXPECT_EQ(out.revision[0], 0); request_config_release(&out); request_config_release(nullptr);
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotRevisionFailureDoesNotReadSettingsOrPublish) {
    request_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
    revision_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(request_config_capture(&out), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(opens, 0); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    revision_error = ESP_OK; EXPECT_EQ(request_config_capture(&out), ESP_OK); request_config_release(&out);
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotEveryProcessingReadFailureStopsWithoutPublication) {
    for (const char *key : expected_keys) {
        request_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
        opens = 0; get_errors[key] = ESP_ERR_NO_MEM;
        EXPECT_EQ(request_config_capture(&out), ESP_ERR_NO_MEM);
        EXPECT_EQ(opens, 1); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(active, 0);
        get_errors.clear();
    }
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotPaletteFailureDoesNotAllocateOrPublishDefault) {
    cJSON_Hooks hooks{json_allocate, json_release}; cJSON_InitHooks(&hooks);
    for (esp_err_t failure : {ESP_FAIL, ESP_ERR_NVS_INVALID_LENGTH, ESP_ERR_NVS_TYPE_MISMATCH}) {
        blob_error = failure;
        request_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
        EXPECT_EQ(request_config_capture(&out), failure);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(json_allocations, 0);
    }
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotEveryJsonAllocationFailureReleasesPartialOwnership) {
    cJSON_Hooks hooks{json_allocate, json_release}; cJSON_InitHooks(&hooks);
    for (const char *mode : {"gc16", "color"}) {
        test_board_display_type = mode; json_fail_at = -1; json_allocations = 0;
        request_config_t success{}; ASSERT_EQ(request_config_capture(&success), ESP_OK);
        int total = json_allocations; EXPECT_EQ(json_live, 2); request_config_release(&success);
        EXPECT_EQ(json_live, 0);
        for (int index = 1; index <= total; ++index) {
            json_fail_at = index; json_allocations = 0;
            request_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
            EXPECT_EQ(request_config_capture(&out), ESP_ERR_NO_MEM) << mode << ':' << index;
            EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(json_live, 0);
        }
    }
    test_board_display_type = "gc16";
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotBusyAndNullDoNotReadOrPublish) {
    request_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
    auto busy = [](void *ctx) -> esp_err_t {
        EXPECT_EQ(request_config_capture((request_config_t *)ctx), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(request_config_capture(nullptr), ESP_ERR_INVALID_ARG); return ESP_OK;
    };
    EXPECT_EQ(config_apply_run(busy, &out), ESP_OK); EXPECT_EQ(revision_calls, 0); EXPECT_EQ(opens, 0);
    EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotHoldsApplicationControlAcrossRevisionAndSettingsReads) {
    probe_options_control = true;
    request_config_t out{}; ASSERT_EQ(request_config_capture(&out), ESP_OK);
    EXPECT_EQ(attempted_writers, 0); EXPECT_EQ(revision_calls, 1); EXPECT_EQ(opens, 2);
    EXPECT_EQ(config_apply_run(attempted_write, nullptr), ESP_OK); request_config_release(&out);
}
TEST_F(ReadPicoSettingsSave, RequestSnapshotRetainsCapturedValuesAfterLaterStorageChanges) {
    request_config_t first{}, second{}; ASSERT_EQ(request_config_capture(&first), ESP_OK);
    std::string processing(first.processing), palette(first.palette), revision(first.revision);
    saved["proc_smode"] = std::string("fit") + '\0'; test_revision = std::numeric_limits<int64_t>::max();
    ASSERT_EQ(request_config_capture(&second), ESP_OK);
    EXPECT_EQ(first.processing, processing); EXPECT_EQ(first.palette, palette); EXPECT_EQ(first.revision, revision);
    EXPECT_STREQ(second.revision, "9223372036854775807"); EXPECT_NE(second.processing, processing);
    request_config_release(&first); request_config_release(&second);
}

namespace {
struct RequestHeaders {
    std::map<std::string, std::string> values;
    int calls = 0, fail_at = -1, stale_set = -1, null_readback_at = -1, lost_set = -1;
    esp_err_t fail_error = ESP_ERR_NO_MEM;
    bool fake_remove = false;
    static esp_err_t remove(void *ctx, const char *key) {
        auto &h = *(RequestHeaders *)ctx;
        if (++h.calls == h.fail_at) return h.fail_error;
        if (h.fake_remove) return ESP_OK;
        return h.values.erase(key) ? ESP_OK : ESP_ERR_NOT_FOUND;
    }
    static esp_err_t get(void *ctx, const char *key, const char **out) {
        auto &h = *(RequestHeaders *)ctx;
        if (++h.calls == h.fail_at) return h.fail_error;
        if (h.calls == h.null_readback_at) { *out = nullptr; return ESP_OK; }
        auto found = h.values.find(key);
        *out = found == h.values.end() ? nullptr : found->second.c_str();
        return found == h.values.end() ? ESP_ERR_NOT_FOUND : ESP_OK;
    }
    static esp_err_t set(void *ctx, const char *key, const char *value) {
        auto &h = *(RequestHeaders *)ctx;
        if (++h.calls == h.fail_at) return h.fail_error;
        if (h.calls == h.lost_set) return ESP_OK;
        h.values[key] = h.calls == h.stale_set ? "wrong" : value; return ESP_OK;
    }
    request_header_ops_t ops() { return {this, remove, get, set}; }
};
}
TEST_F(ReadPicoSettingsSave, RequestHeadersReplaceExistingOrAbsentAndVerifyAllThreeExactValues) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    for (bool existing : {false, true}) {
        RequestHeaders h;
        if (existing) h.values = {{"X-Config-Last-Updated", "old"}, {"X-Processing-Settings", "custom"}, {"X-Color-Palette", "custom"}};
        auto ops = h.ops(); ASSERT_EQ(request_config_set_headers(&snapshot, &ops), ESP_OK);
        EXPECT_EQ(h.calls, 12); EXPECT_EQ(h.values.size(), 3u);
        EXPECT_EQ(h.values["X-Config-Last-Updated"], snapshot.revision);
        EXPECT_EQ(h.values["X-Processing-Settings"], snapshot.processing);
        EXPECT_EQ(h.values["X-Color-Palette"], snapshot.palette);
    }
    request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, RequestHeadersEveryOperationFailureStopsBeforeLaterOperations) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    for (int index = 1; index <= 12; ++index) {
        RequestHeaders h; h.fail_at = index; auto ops = h.ops();
        EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_NO_MEM) << index;
        EXPECT_EQ(h.calls, index);
    }
    request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, RequestHeadersFalseSuccessOrFailedRemovalIsNeverConfirmed) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    for (int index : {3, 7, 11}) {
        RequestHeaders h; h.stale_set = index; auto ops = h.ops();
        EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(h.calls, index + 1);
    }
    RequestHeaders h; h.fake_remove = true; h.values["X-Config-Last-Updated"] = "old"; auto ops = h.ops();
    EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(h.calls, 2);
    request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, RequestHeadersInvalidSnapshotOrOpsRefuseBeforeCallbacks) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    RequestHeaders h; auto ops = h.ops();
    EXPECT_EQ(request_config_set_headers(nullptr, &ops), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(request_config_set_headers(&snapshot, nullptr), ESP_ERR_INVALID_ARG);
    for (int index = 0; index < 3; ++index) {
        auto bad = ops; if (index == 0) bad.remove = nullptr; if (index == 1) bad.get = nullptr; if (index == 2) bad.set = nullptr;
        EXPECT_EQ(request_config_set_headers(&snapshot, &bad), ESP_ERR_INVALID_ARG);
    }
    auto bad = snapshot; bad.processing = nullptr;
    EXPECT_EQ(request_config_set_headers(&bad, &ops), ESP_ERR_INVALID_ARG);
    bad = snapshot; bad.palette = nullptr; EXPECT_EQ(request_config_set_headers(&bad, &ops), ESP_ERR_INVALID_ARG);
    bad = snapshot; memset(bad.revision, 'x', sizeof(bad.revision));
    EXPECT_EQ(request_config_set_headers(&bad, &ops), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(h.calls, 0); request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, HeaderPutAcceptsTokenKeysAndSafeOpaqueValuesIncludingEmpty) {
    RequestHeaders h; auto ops = h.ops();
    for (const char *key : {"Authorization", "If-None-Match", "X-Display-Width", "X!#$%&'*+-.^_`|~09azAZ"})
        for (const char *value : {"", "Bearer synthetic", "W/\"abc-123\"", "a\tb", "{}", "测试"}) {
            EXPECT_EQ(request_header_put_checked(&ops, key, value), ESP_OK);
            EXPECT_EQ(h.values[key], value);
        }
}
TEST_F(ReadPicoSettingsSave, HeaderRemoveConfirmsExistingOrAbsentWithoutSetCallback) {
    for (bool existing : {false, true}) {
        RequestHeaders h; if (existing) h.values["Authorization"] = "synthetic";
        h.values["X-Other"] = "keep";
        auto ops = h.ops(); ops.set = nullptr;
        EXPECT_EQ(request_header_remove_checked(&ops, "Authorization"), ESP_OK);
        EXPECT_EQ(h.calls, 2); EXPECT_EQ(h.values.count("Authorization"), 0u);
        EXPECT_EQ(h.values["X-Other"], "keep");
    }
}
TEST_F(ReadPicoSettingsSave, HeaderRemoveInvalidArgumentsNeverTouchSdk) {
    RequestHeaders h; auto ops = h.ops();
    EXPECT_EQ(request_header_remove_checked(nullptr, "Authorization"), ESP_ERR_INVALID_ARG);
    for (const char *key : {static_cast<const char *>(nullptr), "", "Bad Key", "Bad:Key", "Bad\r\nKey", "中文"})
        EXPECT_EQ(request_header_remove_checked(&ops, key), ESP_ERR_INVALID_ARG);
    auto bad = ops; bad.remove = nullptr;
    EXPECT_EQ(request_header_remove_checked(&bad, "Authorization"), ESP_ERR_INVALID_ARG);
    bad = ops; bad.get = nullptr;
    EXPECT_EQ(request_header_remove_checked(&bad, "Authorization"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(h.calls, 0);
}
TEST_F(ReadPicoSettingsSave, HeaderRemoveRejectsEveryUnexpectedReadbackState) {
    struct Readback { esp_err_t error; const char *value; int calls; };
    for (esp_err_t error : {ESP_OK, ESP_ERR_NOT_FOUND, ESP_FAIL, ESP_ERR_NO_MEM}) {
        for (const char *value : {static_cast<const char *>(nullptr), "synthetic"}) {
            Readback state{error, value, 0};
            request_header_ops_t ops{&state,
                [](void *ctx, const char *) { ++((Readback *)ctx)->calls; return ESP_ERR_NOT_FOUND; },
                [](void *ctx, const char *, const char **out) {
                    auto &s = *(Readback *)ctx; ++s.calls; *out = s.value; return s.error;
                }, nullptr};
            auto expected = error == ESP_ERR_NOT_FOUND && !value ? ESP_OK :
                error != ESP_OK && error != ESP_ERR_NOT_FOUND ? error : ESP_ERR_INVALID_RESPONSE;
            EXPECT_EQ(request_header_remove_checked(&ops, "Authorization"), expected);
            EXPECT_EQ(state.calls, 2);
        }
    }
}
TEST_F(ReadPicoSettingsSave, CredentialStripOptionalOrAuthorizationAliasOnlyRemovesOnce) {
    for (const char *custom : {static_cast<const char *>(nullptr), "", "Authorization", "aUtHoRiZaTiOn", "X-Synthetic-Key"}) {
        RequestHeaders h; auto ops = h.ops(); ops.set = nullptr;
        h.values["Authorization"] = "synthetic";
        h.values["X-Synthetic-Key"] = "custom"; h.values["X-Other"] = "keep";
        bool distinct = custom && !strcmp(custom, "X-Synthetic-Key");
        EXPECT_EQ(request_headers_strip_credentials(&ops, custom), ESP_OK);
        EXPECT_EQ(h.calls, distinct ? 4 : 2); EXPECT_EQ(h.values.count("Authorization"), 0u);
        EXPECT_EQ(h.values.count("X-Synthetic-Key"), distinct ? 0u : 1u);
        EXPECT_EQ(h.values["X-Other"], "keep");
        // Repeated stripping never restores credentials and missing is checked too.
        EXPECT_EQ(request_headers_strip_credentials(&ops, custom), ESP_OK);
        EXPECT_EQ(h.calls, distinct ? 8 : 4);
    }
}
TEST_F(ReadPicoSettingsSave, CredentialStripEachFailurePreservesFirstErrorAndStopsLaterOperations) {
    for (esp_err_t error : {ESP_FAIL, ESP_ERR_NO_MEM, ESP_ERR_INVALID_STATE}) {
        for (int index = 1; index <= 4; ++index) {
            RequestHeaders h; auto ops = h.ops(); ops.set = nullptr;
            h.fail_at = index; h.fail_error = error;
            h.values["Authorization"] = "synthetic";
            h.values["X-Synthetic-Key"] = "custom"; h.values["X-Other"] = "keep";
            EXPECT_EQ(request_headers_strip_credentials(&ops, "X-Synthetic-Key"), error);
            EXPECT_EQ(h.calls, index);
            if (index <= 3) EXPECT_EQ(h.values["X-Synthetic-Key"], "custom");
            EXPECT_EQ(h.values["X-Other"], "keep");
            // Partial removal is not rollback. A deliberate later call can clear
            // what remains, but the failed call never continued or set a secret.
            h.fail_at = -1;
            EXPECT_EQ(request_headers_strip_credentials(&ops, "X-Synthetic-Key"), ESP_OK);
            EXPECT_EQ(h.values.count("Authorization"), 0u);
            EXPECT_EQ(h.values.count("X-Synthetic-Key"), 0u);
        }
    }
}
TEST_F(ReadPicoSettingsSave, CredentialStripInvalidCustomOrOpsRefusesBeforeAnyMutation) {
    RequestHeaders h; auto ops = h.ops(); h.values["Authorization"] = "synthetic";
    for (const char *custom : {"Bad Key", "Bad:Key", "Bad\nKey", "中文"})
        EXPECT_EQ(request_headers_strip_credentials(&ops, custom), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(request_headers_strip_credentials(nullptr, "X-Synthetic-Key"), ESP_ERR_INVALID_ARG);
    auto bad = ops; bad.get = nullptr;
    EXPECT_EQ(request_headers_strip_credentials(&bad, "X-Synthetic-Key"), ESP_ERR_INVALID_ARG);
    bad = ops; bad.remove = nullptr;
    EXPECT_EQ(request_headers_strip_credentials(&bad, "X-Synthetic-Key"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(h.calls, 0); EXPECT_EQ(h.values["Authorization"], "synthetic");
}
TEST_F(ReadPicoSettingsSave, CredentialStripUnconfirmedAuthorizationNeverTouchesCustom) {
    for (bool false_remove : {false, true}) {
        RequestHeaders h; auto ops = h.ops();
        h.values["Authorization"] = "synthetic"; h.values["X-Synthetic-Key"] = "custom";
        h.fake_remove = false_remove;
        if (!false_remove) h.null_readback_at = 2;
        EXPECT_EQ(request_headers_strip_credentials(&ops, "X-Synthetic-Key"), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(h.calls, 2); EXPECT_EQ(h.values["X-Synthetic-Key"], "custom");
    }
}
TEST_F(ReadPicoSettingsSave, HeaderPutRejectsInvalidKeyBytesBeforeAnySdkOperation) {
    for (int byte = 1; byte <= 255; ++byte) {
        bool permitted = (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') ||
                         (byte >= 'a' && byte <= 'z') || strchr("!#$%&'*+-.^_`|~", byte);
        if (permitted) continue;
        std::string key = "X"; key += (char)byte;
        RequestHeaders h; auto ops = h.ops();
        EXPECT_EQ(request_header_put_checked(&ops, key.c_str(), "synthetic"), ESP_ERR_INVALID_ARG) << byte;
        EXPECT_EQ(h.calls, 0);
    }
}
TEST_F(ReadPicoSettingsSave, HeaderPutRejectsControlOrNewlineValuesBeforeAnySdkOperation) {
    for (int byte = 1; byte <= 127; ++byte) {
        if (byte >= 32 && byte != 127) continue;
        if (byte == '\t') continue;
        std::string value = "safe"; value += (char)byte; value += "Injected: other";
        RequestHeaders h; auto ops = h.ops();
        EXPECT_EQ(request_header_put_checked(&ops, "Authorization", value.c_str()), ESP_ERR_INVALID_ARG) << byte;
        EXPECT_EQ(h.calls, 0);
    }
}
TEST_F(ReadPicoSettingsSave, HeaderPutNullEmptyKeyOrCallbacksRejectWithoutMutation) {
    RequestHeaders h; auto ops = h.ops();
    EXPECT_EQ(request_header_put_checked(nullptr, "X", "v"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(request_header_put_checked(&ops, nullptr, "v"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(request_header_put_checked(&ops, "", "v"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(request_header_put_checked(&ops, "X", nullptr), ESP_ERR_INVALID_ARG);
    for (int index = 0; index < 3; ++index) {
        auto bad = ops; if (index == 0) bad.remove = nullptr; if (index == 1) bad.get = nullptr; if (index == 2) bad.set = nullptr;
        EXPECT_EQ(request_header_put_checked(&bad, "X", "v"), ESP_ERR_INVALID_ARG);
    }
    EXPECT_EQ(h.calls, 0);
}
TEST_F(ReadPicoSettingsSave, HeaderPutPreservesFirstSdkErrorAndStopsAtThatOperation) {
    for (int index = 1; index <= 4; ++index) {
        RequestHeaders h; h.fail_at = index; auto ops = h.ops();
        EXPECT_EQ(request_header_put_checked(&ops, "Authorization", "Bearer synthetic"), ESP_ERR_NO_MEM);
        EXPECT_EQ(h.calls, index);
    }
}
TEST_F(ReadPicoSettingsSave, RequestHeadersNullOrMissingReadbackDespiteSuccessRefusesRequest) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    for (int index : {4, 8, 12}) {
        RequestHeaders h; h.null_readback_at = index; auto ops = h.ops();
        EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(h.calls, index);
    }
    for (int index : {3, 7, 11}) {
        RequestHeaders h; h.lost_set = index; auto ops = h.ops();
        EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_NOT_FOUND);
        EXPECT_EQ(h.calls, index + 1);
    }
    request_config_release(&snapshot);
}

#if READ_PICO_REQUEST_HEADER_SDK_TESTS
extern "C" {
void *request_test_calloc(size_t n, size_t size) {
    if (++sdk_header_allocations == sdk_header_fail_at) return nullptr;
    void *p = calloc(n, size); if (p) sdk_header_live.insert(p); return p;
}
void *request_test_realloc(void *p, size_t size) {
    if (++sdk_header_allocations == sdk_header_fail_at) return nullptr;
    auto out = realloc(p, size);
    if (out) { sdk_header_live.erase(p); sdk_header_live.insert(out); }
    return out;
}
char *request_test_strdup(const char *s) {
    if (++sdk_header_allocations == sdk_header_fail_at) return nullptr;
    auto out = strdup(s); if (out) sdk_header_live.insert(out); return out;
}
void request_test_free(void *p) {
    if (p) EXPECT_EQ(sdk_header_live.erase(p), 1u);
    free(p);
}
}
namespace {
request_header_ops_t sdk_header_ops(http_header_handle_t header) {
    return {header,
        [](void *h, const char *key) { return http_header_delete((http_header_handle_t)h, key); },
        [](void *h, const char *key, const char **out) {
            char *borrowed = nullptr; auto err = http_header_get((http_header_handle_t)h, key, &borrowed);
            *out = borrowed; return err;
        },
        [](void *h, const char *key, const char *value) { return http_header_set((http_header_handle_t)h, key, value); }};
}
}
TEST_F(ReadPicoSettingsSave, FixedSdkExistingHeaderUpdateAllocationFailureReturnsSuccessButNull) {
    auto h = http_header_init(); ASSERT_NE(h, nullptr);
    ASSERT_EQ(http_header_set(h, "X-Config-Last-Updated", "old"), ESP_OK);
    sdk_header_allocations = 0; sdk_header_fail_at = 1;
    EXPECT_EQ(http_header_set(h, "X-Config-Last-Updated", "new"), ESP_OK);
    char *value = (char *)"sentinel";
    EXPECT_EQ(http_header_get(h, "X-Config-Last-Updated", &value), ESP_OK); EXPECT_EQ(value, nullptr);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
TEST_F(ReadPicoSettingsSave, FixedSdkCheckedReplacementRejectsEveryNewHeaderAllocationFailure) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    for (bool existing : {false, true}) for (int position = 1; position <= 9; ++position) {
        sdk_header_fail_at = -1;
        auto h = http_header_init(); ASSERT_NE(h, nullptr);
        if (existing) for (const char *key : {"X-Config-Last-Updated", "X-Processing-Settings", "X-Color-Palette"})
            ASSERT_EQ(http_header_set(h, key, "old"), ESP_OK);
        sdk_header_allocations = 0; sdk_header_fail_at = position;
        auto ops = sdk_header_ops(h);
        EXPECT_EQ(request_config_set_headers(&snapshot, &ops), ESP_ERR_NO_MEM) << existing << ':' << position;
        EXPECT_EQ(sdk_header_allocations, position); EXPECT_EQ(http_header_destroy(h), ESP_OK);
        EXPECT_TRUE(sdk_header_live.empty());
    }
    request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, FixedSdkHeaderValuesOwnCopiesAfterSnapshotRelease) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    std::string revision(snapshot.revision), processing(snapshot.processing), palette(snapshot.palette);
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h);
    ASSERT_EQ(request_config_set_headers(&snapshot, &ops), ESP_OK); request_config_release(&snapshot);
    char *value = nullptr;
    ASSERT_EQ(http_header_get(h, "X-Config-Last-Updated", &value), ESP_OK); EXPECT_EQ(revision, value);
    ASSERT_EQ(http_header_get(h, "X-Processing-Settings", &value), ESP_OK); EXPECT_EQ(processing, value);
    ASSERT_EQ(http_header_get(h, "X-Color-Palette", &value), ESP_OK); EXPECT_EQ(palette, value);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
TEST_F(ReadPicoSettingsSave, FixedSdkWireHeadersContainExactCompactJsonAndRevision) {
    request_config_t snapshot{}; ASSERT_EQ(request_config_capture(&snapshot), ESP_OK);
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h);
    ASSERT_EQ(request_config_set_headers(&snapshot, &ops), ESP_OK);
    char bytes[2048] = {}; int size = sizeof(bytes);
    EXPECT_EQ(http_header_generate_string(h, 0, bytes, &size), 3);
    std::string expected = std::string("X-Config-Last-Updated: ") + snapshot.revision + "\r\nX-Processing-Settings: " +
                           snapshot.processing + "\r\nX-Color-Palette: " + snapshot.palette + "\r\n\r\n";
    EXPECT_EQ(std::string(bytes), expected);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty()); request_config_release(&snapshot);
}
TEST_F(ReadPicoSettingsSave, FixedSdkHeaderPutRejectsSilentWhitespaceTrimmingAndKeepsExactInteriorTab) {
    for (const char *value : {" value", "value ", "\tvalue", "value\t", " "}) {
        auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h);
        EXPECT_EQ(request_header_put_checked(&ops, "Authorization", value), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
    }
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h);
    EXPECT_EQ(request_header_put_checked(&ops, "X-Custom", "a\tb"), ESP_OK);
    EXPECT_EQ(request_header_put_checked(&ops, "X-Custom", ""), ESP_OK);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
TEST_F(ReadPicoSettingsSave, FixedSdkHeaderPutReplacesCredentialWithCheckedAllocationAndOwnedCopy) {
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h);
    ASSERT_EQ(http_header_set(h, "Authorization", "old"), ESP_OK);
    char value[] = "Bearer synthetic";
    ASSERT_EQ(request_header_put_checked(&ops, "Authorization", value), ESP_OK);
    memset(value, 'x', strlen(value));
    char *stored = nullptr; ASSERT_EQ(http_header_get(h, "Authorization", &stored), ESP_OK);
    EXPECT_STREQ(stored, "Bearer synthetic");
    sdk_header_allocations = 0; sdk_header_fail_at = 1;
    EXPECT_EQ(request_header_put_checked(&ops, "Authorization", "Bearer next"), ESP_ERR_NO_MEM);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
TEST_F(ReadPicoSettingsSave, FixedSdkCredentialStripIsCaseInsensitiveAllocationFreeAndWireSecretsAbsent) {
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h); ops.set = nullptr;
    ASSERT_EQ(http_header_set(h, "authorization", "synthetic-bearer"), ESP_OK);
    ASSERT_EQ(http_header_set(h, "x-synthetic-key", "synthetic-custom"), ESP_OK);
    ASSERT_EQ(http_header_set(h, "X-Other", "keep"), ESP_OK);
    sdk_header_allocations = 0; sdk_header_fail_at = 1;
    EXPECT_EQ(request_headers_strip_credentials(&ops, "X-Synthetic-Key"), ESP_OK);
    EXPECT_EQ(sdk_header_allocations, 0);
    char bytes[256] = {}; int size = sizeof(bytes);
    EXPECT_EQ(http_header_generate_string(h, 0, bytes, &size), 1);
    EXPECT_STREQ(bytes, "X-Other: keep\r\n\r\n");
    EXPECT_EQ(request_headers_strip_credentials(&ops, "AUTHORIZATION"), ESP_OK);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
TEST_F(ReadPicoSettingsSave, FixedSdkHeaderRemoveClearsNullValueLeftByUncheckedUpdate) {
    auto h = http_header_init(); ASSERT_NE(h, nullptr); auto ops = sdk_header_ops(h); ops.set = nullptr;
    ASSERT_EQ(http_header_set(h, "Authorization", "synthetic"), ESP_OK);
    sdk_header_allocations = 0; sdk_header_fail_at = 1;
    ASSERT_EQ(http_header_set(h, "Authorization", "next"), ESP_OK);
    char *value = (char *)"sentinel";
    ASSERT_EQ(http_header_get(h, "Authorization", &value), ESP_OK); EXPECT_EQ(value, nullptr);
    EXPECT_EQ(request_header_remove_checked(&ops, "authorization"), ESP_OK);
    EXPECT_EQ(http_header_get(h, "Authorization", &value), ESP_ERR_NOT_FOUND); EXPECT_EQ(value, nullptr);
    EXPECT_EQ(http_header_destroy(h), ESP_OK); EXPECT_TRUE(sdk_header_live.empty());
}
#endif

TEST_F(ReadPicoSettingsSave, ProcessingSuccessChecksAllFieldsInOrderAndCommitsOnce) {
    processing_settings_t settings; processing_settings_get_defaults(&settings);
    EXPECT_EQ(processing_settings_save(&settings), ESP_OK);
    EXPECT_EQ(keys, (std::vector<std::string>(std::begin(expected_keys), std::end(expected_keys))));
    EXPECT_EQ(sets, 13); EXPECT_EQ(commits, 1); EXPECT_EQ(closes, 1);
}
TEST_F(ReadPicoSettingsSave, EachProcessingSetFailureClosesWithoutCommitOrLaterFields) {
    processing_settings_t settings; processing_settings_get_defaults(&settings);
    for (int index = 1; index <= 13; ++index) {
        sets = commits = closes = 0; keys.clear(); saved.clear(); fail_set = index;
        EXPECT_EQ(processing_settings_save(&settings), ESP_ERR_NO_MEM) << index;
        EXPECT_EQ(sets, index); EXPECT_EQ(commits, 0); EXPECT_EQ(closes, 1); EXPECT_EQ(active, 0);
        EXPECT_EQ(saved.size(), (size_t)(index - 1)); // Earlier sets may already have persisted.
    }
}
TEST_F(ReadPicoSettingsSave, ProcessingOpenAndCommitFailureNeverReportSuccess) {
    processing_settings_t settings; processing_settings_get_defaults(&settings); open_error = ESP_FAIL;
    EXPECT_EQ(processing_settings_save(&settings), ESP_FAIL); EXPECT_EQ(sets, 0); EXPECT_EQ(closes, 0);
    open_error = ESP_OK; commit_error = ESP_FAIL;
    EXPECT_EQ(processing_settings_save(&settings), ESP_FAIL); EXPECT_EQ(sets, 13); EXPECT_EQ(commits, 1); EXPECT_EQ(closes, 1);
}
TEST_F(ReadPicoSettingsSave, ProcessingNullAndNonfiniteFieldsRefuseBeforeIo) {
    EXPECT_EQ(processing_settings_save(nullptr), ESP_ERR_INVALID_ARG);
    for (unsigned field = 0; field < 7; ++field) {
        processing_settings_t settings; processing_settings_get_defaults(&settings);
        float *values[] = {&settings.exposure, &settings.saturation, &settings.contrast, &settings.strength,
                          &settings.shadow_boost, &settings.highlight_compress, &settings.midpoint};
        *values[field] = std::numeric_limits<float>::infinity();
        EXPECT_EQ(processing_settings_save(&settings), ESP_ERR_INVALID_ARG);
    }
    EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, ProcessingUnterminatedEachStringRefusesBeforeIo) {
    for (unsigned field = 0; field < 5; ++field) {
        processing_settings_t settings; processing_settings_get_defaults(&settings);
        char *values[] = {settings.tone_mode, settings.color_method, settings.dither_algorithm,
                         settings.scale_mode, settings.background_color};
        size_t sizes[] = {sizeof(settings.tone_mode), sizeof(settings.color_method), sizeof(settings.dither_algorithm),
                          sizeof(settings.scale_mode), sizeof(settings.background_color)};
        memset(values[field], 'x', sizes[field]); EXPECT_EQ(processing_settings_save(&settings), ESP_ERR_INVALID_SIZE);
    }
    EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, FiniteDoubleThatOverflowsFloatCannotBeSaved) {
    cJSON *json = cJSON_Parse("{\"exposure\":1e100}"); ASSERT_NE(json, nullptr);
    processing_settings_t settings; processing_settings_get_defaults(&settings);
    processing_settings_from_json(json, &settings); cJSON_Delete(json);
    EXPECT_EQ(processing_settings_save(&settings), ESP_ERR_INVALID_ARG); EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, PaletteMissingNamespaceOrKeyOnlyUsesDefaults) {
    color_palette_t out, defaults; color_palette_get_defaults(&defaults);
    open_error = ESP_ERR_NVS_NOT_FOUND;
    EXPECT_EQ(color_palette_load(&out), ESP_OK); EXPECT_EQ(out.gray_gamma, defaults.gray_gamma); EXPECT_EQ(closes, 0);
    open_error = ESP_OK; EXPECT_EQ(color_palette_load(&out), ESP_OK); EXPECT_EQ(closes, 1);
    EXPECT_EQ(out.white.r, defaults.white.r);
}
TEST_F(ReadPicoSettingsSave, PaletteReadIoTypeAndLengthErrorsDoNotFallbackOrChangeOutput) {
    color_palette_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
    open_error = ESP_FAIL; EXPECT_EQ(color_palette_load(&out), ESP_FAIL); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    open_error = ESP_OK;
    for (esp_err_t error : {ESP_FAIL, ESP_ERR_NVS_TYPE_MISMATCH, ESP_ERR_NVS_INVALID_LENGTH}) {
        blob_error = error; EXPECT_EQ(color_palette_load(&out), error);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    }
}
TEST_F(ReadPicoSettingsSave, PaletteExactSizeRequiredAndSuccessfulCopyIsIndependent) {
    color_palette_t palette; color_palette_get_defaults(&palette); ASSERT_EQ(color_palette_save(&palette), ESP_OK);
    color_palette_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
    short_blob = true; EXPECT_EQ(color_palette_load(&out), ESP_ERR_INVALID_SIZE); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    short_blob = false; ASSERT_EQ(color_palette_load(&out), ESP_OK); EXPECT_EQ(out.gray_gamma, palette.gray_gamma);
    saved.clear(); EXPECT_EQ(out.gray_gamma, palette.gray_gamma);
}
TEST_F(ReadPicoSettingsSave, PaletteNonfiniteInputAndRecordRefuseWithoutPublication) {
    color_palette_t palette; color_palette_get_defaults(&palette); palette.gray_gamma = std::numeric_limits<float>::quiet_NaN();
    EXPECT_EQ(color_palette_save(&palette), ESP_ERR_INVALID_ARG); EXPECT_EQ(opens, 0);
    saved["palette"] = std::string((char *)&palette, sizeof(palette)); color_palette_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
    EXPECT_EQ(color_palette_load(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    EXPECT_EQ(color_palette_load(nullptr), ESP_ERR_INVALID_ARG); EXPECT_EQ(color_palette_save(nullptr), ESP_ERR_INVALID_ARG);
}
TEST_F(ReadPicoSettingsSave, PaletteSaveSetAndCommitErrorsPropagateAndClose) {
    color_palette_t palette; color_palette_get_defaults(&palette); fail_set = 1;
    EXPECT_EQ(color_palette_save(&palette), ESP_ERR_NO_MEM); EXPECT_EQ(commits, 0); EXPECT_EQ(closes, 1);
    fail_set = -1; commit_error = ESP_FAIL;
    EXPECT_EQ(color_palette_save(&palette), ESP_FAIL); EXPECT_EQ(commits, 1); EXPECT_EQ(closes, 2);
}

static const float kRamp[COLOR_PALETTE_GRAY_LEVELS] = {
    0.028f, 0.034f, 0.042f, 0.065f, 0.095f, 0.132f, 0.201f, 0.253f,
    0.337f, 0.394f, 0.470f, 0.522f, 0.581f, 0.610f, 0.643f, 0.645f};
TEST_F(ReadPicoSettingsSave, GrayLevelsValidationRejectsBadRamps) {
    EXPECT_TRUE(color_palette_gray_levels_valid(kRamp));
    float bad[COLOR_PALETTE_GRAY_LEVELS];
    memcpy(bad, kRamp, sizeof(bad)); bad[5] = bad[4] - 0.01f; EXPECT_FALSE(color_palette_gray_levels_valid(bad));
    memcpy(bad, kRamp, sizeof(bad)); bad[0] = 0.0f; EXPECT_FALSE(color_palette_gray_levels_valid(bad));
    memcpy(bad, kRamp, sizeof(bad)); bad[15] = 1.5f; EXPECT_FALSE(color_palette_gray_levels_valid(bad));
    memcpy(bad, kRamp, sizeof(bad)); bad[7] = std::numeric_limits<float>::quiet_NaN(); EXPECT_FALSE(color_palette_gray_levels_valid(bad));
    for (float &v : bad) v = 0.5f; EXPECT_FALSE(color_palette_gray_levels_valid(bad));
    EXPECT_FALSE(color_palette_gray_levels_valid(nullptr));
    EXPECT_EQ(color_palette_save_gray_levels(bad), ESP_ERR_INVALID_ARG); EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, GrayLevelsMissingMeansAbsentWithoutChangingOutput) {
    float out[COLOR_PALETTE_GRAY_LEVELS]; memset(out, 0x55, sizeof(out)); bool present = true;
    open_error = ESP_ERR_NVS_NOT_FOUND;
    EXPECT_EQ(color_palette_load_gray_levels(out, &present), ESP_OK); EXPECT_FALSE(present);
    open_error = ESP_OK; present = true;
    EXPECT_EQ(color_palette_load_gray_levels(out, &present), ESP_OK); EXPECT_FALSE(present);
    unsigned char raw[sizeof(out)]; memset(raw, 0x55, sizeof(raw)); EXPECT_EQ(memcmp(out, raw, sizeof(out)), 0);
}
TEST_F(ReadPicoSettingsSave, GrayLevelsRoundTripAndEraseLeavesPaletteBlobAlone) {
    color_palette_t palette; color_palette_get_defaults(&palette); ASSERT_EQ(color_palette_save(&palette), ESP_OK);
    std::string blob = saved["palette"];
    ASSERT_EQ(color_palette_save_gray_levels(kRamp), ESP_OK);
    EXPECT_EQ(saved["gray_levels"].size(), sizeof(kRamp)); EXPECT_EQ(saved["palette"], blob);
    float out[COLOR_PALETTE_GRAY_LEVELS]; bool present = false;
    ASSERT_EQ(color_palette_load_gray_levels(out, &present), ESP_OK); EXPECT_TRUE(present);
    EXPECT_EQ(memcmp(out, kRamp, sizeof(out)), 0);
    ASSERT_EQ(color_palette_save_gray_levels(nullptr), ESP_OK); EXPECT_EQ(saved.count("gray_levels"), 0u);
    EXPECT_EQ(color_palette_save_gray_levels(nullptr), ESP_OK); // already absent is still success
    EXPECT_EQ(saved["palette"], blob);
}
TEST_F(ReadPicoSettingsSave, GrayLevelsCorruptRecordIsAnErrorNotADefault) {
    float out[COLOR_PALETTE_GRAY_LEVELS]; memset(out, 0x55, sizeof(out)); auto before = std::string((char *)out, sizeof(out));
    bool present = false;
    saved["gray_levels"] = std::string((const char *)kRamp, sizeof(kRamp) - 4);
    EXPECT_EQ(color_palette_load_gray_levels(out, &present), ESP_ERR_INVALID_SIZE);
    float bad[COLOR_PALETTE_GRAY_LEVELS]; memcpy(bad, kRamp, sizeof(bad)); bad[3] = 2.0f;
    saved["gray_levels"] = std::string((const char *)bad, sizeof(bad));
    EXPECT_EQ(color_palette_load_gray_levels(out, &present), ESP_ERR_INVALID_RESPONSE);
    blob_error = ESP_FAIL; EXPECT_EQ(color_palette_load_gray_levels(out, &present), ESP_FAIL);
    EXPECT_EQ(std::string((char *)out, sizeof(out)), before); EXPECT_FALSE(present);
}
TEST_F(ReadPicoSettingsSave, GrayLevelsWriteAndEraseErrorsPropagate) {
    fail_set = 1; EXPECT_EQ(color_palette_save_gray_levels(kRamp), ESP_ERR_NO_MEM); EXPECT_EQ(commits, 0);
    fail_set = -1; erase_error = ESP_FAIL; EXPECT_EQ(color_palette_save_gray_levels(nullptr), ESP_FAIL);
    erase_error = ESP_OK; commit_error = ESP_FAIL; EXPECT_EQ(color_palette_save_gray_levels(kRamp), ESP_FAIL);
    EXPECT_EQ(closes, 3);
}
TEST_F(ReadPicoSettingsSave, GrayLevelsJsonKeepClearSetAndRejectInvalid) {
    color_palette_gray_levels_action_t action = COLOR_PALETTE_GRAY_LEVELS_SET;
    float out[COLOR_PALETTE_GRAY_LEVELS] = {0};
    cJSON *json = cJSON_Parse("{\"gamma\":0.8}");
    EXPECT_EQ(color_palette_gray_levels_from_json(json, &action, out), ESP_OK);
    EXPECT_EQ(action, COLOR_PALETTE_GRAY_LEVELS_KEEP); cJSON_Delete(json);
    json = cJSON_Parse("{\"levels_y\":null}");
    EXPECT_EQ(color_palette_gray_levels_from_json(json, &action, out), ESP_OK);
    EXPECT_EQ(action, COLOR_PALETTE_GRAY_LEVELS_CLEAR); cJSON_Delete(json);
    json = cJSON_Parse("{\"levels_y\":[0.028,0.034,0.042,0.065,0.095,0.132,0.201,0.253,0.337,0.394,0.47,0.522,0.581,0.61,0.643,0.645]}");
    EXPECT_EQ(color_palette_gray_levels_from_json(json, &action, out), ESP_OK);
    EXPECT_EQ(action, COLOR_PALETTE_GRAY_LEVELS_SET); EXPECT_FLOAT_EQ(out[6], 0.201f); cJSON_Delete(json);
    for (const char *bad : {"{\"levels_y\":[0.1,0.2]}", "{\"levels_y\":\"x\"}",
                            "{\"levels_y\":[0.5,0.4,0.42,0.065,0.095,0.132,0.201,0.253,0.337,0.394,0.47,0.522,0.581,0.61,0.643,0.645]}",
                            "{\"levels_y\":[0.028,0.034,0.042,0.065,0.095,0.132,0.201,0.253,0.337,0.394,0.47,0.522,0.581,0.61,0.643,1e100]}"}) {
        json = cJSON_Parse(bad); ASSERT_NE(json, nullptr);
        action = COLOR_PALETTE_GRAY_LEVELS_KEEP; float keep[COLOR_PALETTE_GRAY_LEVELS]; memcpy(keep, out, sizeof(keep));
        EXPECT_EQ(color_palette_gray_levels_from_json(json, &action, out), ESP_ERR_INVALID_ARG) << bad;
        EXPECT_EQ(action, COLOR_PALETTE_GRAY_LEVELS_KEEP); EXPECT_EQ(memcmp(out, keep, sizeof(out)), 0);
        cJSON_Delete(json);
    }
}
TEST_F(ReadPicoSettingsSave, GrayLevelsReportedInPaletteJsonOnlyWhenGiven) {
    color_palette_t palette; color_palette_get_defaults(&palette);
    char *plain = color_palette_to_json_with_levels(&palette, nullptr); ASSERT_NE(plain, nullptr);
    cJSON *a = cJSON_Parse(plain); free(plain); ASSERT_NE(a, nullptr);
    EXPECT_EQ(cJSON_GetObjectItem(a, "levels_y"), nullptr); cJSON_Delete(a);
    char *with = color_palette_to_json_with_levels(&palette, kRamp); ASSERT_NE(with, nullptr);
    cJSON *b = cJSON_Parse(with); free(with); ASSERT_NE(b, nullptr);
    cJSON *levels = cJSON_GetObjectItem(b, "levels_y"); ASSERT_TRUE(cJSON_IsArray(levels));
    EXPECT_EQ(cJSON_GetArraySize(levels), 16); EXPECT_NEAR(cJSON_GetArrayItem(levels, 15)->valuedouble, 0.645, 1e-6);
    cJSON_Delete(b);
    float bad[COLOR_PALETTE_GRAY_LEVELS]; memcpy(bad, kRamp, sizeof(bad)); bad[2] = 0.9f;
    EXPECT_EQ(color_palette_to_json_with_levels(&palette, bad), nullptr);
}

TEST_F(ReadPicoSettingsSave, ProcessingLoadMissingNamespaceAndFieldsUseDefaultsWithoutWrites) {
    processing_settings_t out, defaults; processing_settings_get_defaults(&defaults);
    open_error = ESP_ERR_NVS_NOT_FOUND;
    EXPECT_EQ(processing_settings_load(&out), ESP_OK); EXPECT_STREQ(out.tone_mode, defaults.tone_mode);
    EXPECT_EQ(closes, 0); EXPECT_EQ(read_calls, 0);
    open_error = ESP_OK; EXPECT_EQ(processing_settings_load(&out), ESP_OK);
    EXPECT_EQ(memcmp(&out, &defaults, sizeof(out)), 0); EXPECT_EQ(read_calls, 13); EXPECT_EQ(closes, 1);
    EXPECT_EQ(sets, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ReadPicoSettingsSave, ProcessingLoadSuccessfulRoundTripPublishesCompleteIndependentCandidate) {
    processing_settings_t original; processing_settings_get_defaults(&original);
    original.exposure = 1.7f; original.saturation = .9f; original.compress_dynamic_range = false;
    strcpy(original.tone_mode, "scurve"); strcpy(original.color_method, "lab");
    strcpy(original.dither_algorithm, "sierra"); strcpy(original.scale_mode, "fit"); strcpy(original.background_color, "black");
    ASSERT_EQ(processing_settings_save(&original), ESP_OK);
    processing_settings_t out; memset(&out, 0x55, sizeof(out));
    ASSERT_EQ(processing_settings_load(&out), ESP_OK); EXPECT_EQ(memcmp(&out, &original, sizeof(out)), 0);
    saved.clear(); EXPECT_STREQ(out.tone_mode, "scurve"); EXPECT_FLOAT_EQ(out.exposure, 1.7f);
}
TEST_F(ReadPicoSettingsSave, ProcessingLoadOpenFailureDoesNotPublishDefaultsOrPerformReads) {
    processing_settings_t out; memset(&out, 0x55, sizeof(out)); auto before = out; open_error = ESP_FAIL;
    EXPECT_EQ(processing_settings_load(&out), ESP_FAIL); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    EXPECT_EQ(read_calls, 0); EXPECT_EQ(closes, 0); EXPECT_EQ(processing_settings_load(nullptr), ESP_ERR_INVALID_ARG);
}
TEST_F(ReadPicoSettingsSave, EachProcessingGetErrorStopsAndDoesNotPublishPartialOrDefaultValues) {
    processing_settings_t original; processing_settings_get_defaults(&original); original.exposure = 2;
    ASSERT_EQ(processing_settings_save(&original), ESP_OK);
    for (auto key : expected_keys) for (auto error : {ESP_FAIL, ESP_ERR_NVS_TYPE_MISMATCH, ESP_ERR_NVS_INVALID_LENGTH}) {
        get_errors.clear(); get_errors[key] = error; processing_settings_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
        int closed_before = closes;
        EXPECT_EQ(processing_settings_load(&out), error) << key;
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(closes, closed_before + 1);
    }
}
TEST_F(ReadPicoSettingsSave, EachNonfiniteProcessingRecordRefusesPublication) {
    for (auto key : {"proc_exp", "proc_sat", "proc_cont", "proc_str", "proc_shad", "proc_high", "proc_mid"}) {
        saved.clear(); float bad = std::numeric_limits<float>::quiet_NaN(); saved[key] = std::string((char *)&bad, sizeof(bad));
        processing_settings_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
        EXPECT_EQ(processing_settings_load(&out), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    }
}
TEST_F(ReadPicoSettingsSave, ProcessingStoredBooleanMustBeZeroOrOne) {
    saved["proc_cdr"] = std::string(1, '\2'); processing_settings_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
    EXPECT_EQ(processing_settings_load(&out), ESP_ERR_INVALID_RESPONSE); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
}
TEST_F(ReadPicoSettingsSave, MalformedAndOversizedProcessingStringsDoNotChangeCallerOutput) {
    for (auto key : {"proc_tone", "proc_col", "proc_dith", "proc_smode", "proc_bgcol"})
      for (auto value : {std::string(), std::string("abc"), std::string("x\0y\0", 4), std::string(30, 'x') + '\0'}) {
        saved.clear(); saved[key] = value; processing_settings_t out; memset(&out, 0x55, sizeof(out)); auto before = out;
        EXPECT_NE(processing_settings_load(&out), ESP_OK); EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    }
}
TEST_F(ReadPicoSettingsSave, ProcessingCheckedJsonValidFieldsPreserveUnselectedCandidate) {
    processing_settings_t out; processing_settings_get_defaults(&out); out.contrast = 1.3f;
    cJSON *root = cJSON_Parse("{\"exposure\":2,\"toneMode\":\"scurve\",\"colorMethod\":\"lab\",\"ditherAlgorithm\":\"stucki\",\"scaleMode\":\"cover\",\"backgroundColor\":\"black\",\"compressDynamicRange\":false,\"unknown\":1}");
    ASSERT_NE(root, nullptr); EXPECT_EQ(processing_settings_from_json_checked(root, &out), ESP_OK); cJSON_Delete(root);
    EXPECT_FLOAT_EQ(out.exposure, 2); EXPECT_FLOAT_EQ(out.contrast, 1.3f); EXPECT_STREQ(out.tone_mode, "scurve");
    EXPECT_FALSE(out.compress_dynamic_range); EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, EveryProcessingWrongFieldTypeRefusesWithoutPartialCandidate) {
    for (auto key : {"exposure", "saturation", "contrast", "strength", "shadowBoost", "highlightCompress", "midpoint",
                     "toneMode", "colorMethod", "ditherAlgorithm", "scaleMode", "backgroundColor", "compressDynamicRange"}) {
        processing_settings_t out; processing_settings_get_defaults(&out); auto before = out;
        cJSON *root = cJSON_CreateObject(); cJSON_AddNumberToObject(root, "unknown", 2); cJSON_AddNullToObject(root, key);
        EXPECT_EQ(processing_settings_from_json_checked(root, &out), ESP_ERR_INVALID_ARG) << key;
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); cJSON_Delete(root);
    }
}
TEST_F(ReadPicoSettingsSave, ProcessingInvalidEnumOrTruncationRefusesEvenAfterValidNumber) {
    for (auto key : {"toneMode", "colorMethod", "ditherAlgorithm", "scaleMode", "backgroundColor"})
        for (auto value : {"", "unknown", "abcdefghijklmnopqrstuvwxyz0123456789"}) {
            processing_settings_t out; processing_settings_get_defaults(&out); auto before = out;
            cJSON *root = cJSON_CreateObject(); cJSON_AddNumberToObject(root, "exposure", 2); cJSON_AddStringToObject(root, key, value);
            EXPECT_NE(processing_settings_from_json_checked(root, &out), ESP_OK);
            EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); cJSON_Delete(root);
        }
}
TEST_F(ReadPicoSettingsSave, CheckedJsonFiniteDoubleOverflowAndWrongRootsRefuseWithoutIo) {
    processing_settings_t out; processing_settings_get_defaults(&out); auto before = out;
    cJSON *root = cJSON_Parse("{\"exposure\":1e100}");
    EXPECT_EQ(processing_settings_from_json_checked(root, &out), ESP_ERR_INVALID_ARG); cJSON_Delete(root);
    EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    root = cJSON_Parse("[]"); EXPECT_EQ(processing_settings_from_json_checked(root, &out), ESP_ERR_INVALID_ARG);
    color_palette_t palette; color_palette_get_defaults(&palette);
    EXPECT_EQ(color_palette_from_json_checked(root, &palette), ESP_ERR_INVALID_ARG); cJSON_Delete(root);
    EXPECT_EQ(processing_settings_from_json_checked(nullptr, &out), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(color_palette_from_json_checked(nullptr, &palette), ESP_ERR_INVALID_ARG); EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, PaletteCheckedJsonValidSelectedComponentsAndCalibrationPreserveOtherValues) {
    color_palette_t out; color_palette_get_defaults(&out); auto before = out;
    cJSON *root = cJSON_Parse("{\"black\":{\"r\":0,\"g\":255},\"gamma\":1.42,\"white_y\":0.65,\"unknown\":true}");
    ASSERT_NE(root, nullptr); EXPECT_EQ(color_palette_from_json_checked(root, &out), ESP_OK); cJSON_Delete(root);
    EXPECT_EQ(out.black.r, 0); EXPECT_EQ(out.black.g, 255); EXPECT_EQ(out.black.b, before.black.b);
    EXPECT_EQ(out.white.r, before.white.r); EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoSettingsSave, EveryPaletteColorAndComponentRefusesWrapTruncationAndWrongTypes) {
    for (auto name : {"black", "white", "yellow", "red", "blue", "green"}) for (auto component : {"r", "g", "b"})
        for (auto value : {"-1", "256", "1.5", "\"1\"", "null"}) {
            color_palette_t out; color_palette_get_defaults(&out); auto before = out;
            std::string json = std::string("{\"gamma\":2,\"") + name + "\":{\"" + component + "\":" + value + "}}";
            cJSON *root = cJSON_Parse(json.c_str()); ASSERT_NE(root, nullptr);
            EXPECT_EQ(color_palette_from_json_checked(root, &out), ESP_ERR_INVALID_ARG);
            EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); cJSON_Delete(root);
        }
}
TEST_F(ReadPicoSettingsSave, PaletteInvalidColorObjectAndCalibrationTypeOrOverflowRefuse) {
    for (auto json : {"{\"black\":null}", "{\"red\":[]}", "{\"gamma\":\"1\"}",
                      "{\"gamma\":1e100}", "{\"black_y\":null}", "{\"white_y\":true}"}) {
        color_palette_t out; color_palette_get_defaults(&out); auto before = out; cJSON *root = cJSON_Parse(json);
        ASSERT_NE(root, nullptr); EXPECT_EQ(color_palette_from_json_checked(root, &out), ESP_ERR_INVALID_ARG);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); cJSON_Delete(root);
    }
}
TEST_F(ReadPicoSettingsSave, EverySerializerAllocationFailureRefusesPartialJsonAndLeaksNothing) {
    processing_settings_t settings; processing_settings_get_defaults(&settings);
    color_palette_t palette; color_palette_get_defaults(&palette);
    cJSON_Hooks hooks{json_allocate, json_release}; cJSON_InitHooks(&hooks);
    for (int mode = 0; mode < 3; ++mode) {
        test_board_display_type = mode == 2 ? "color" : "gc16";
        auto serialize = [&]() { return mode == 0 ? processing_settings_to_json(&settings) : color_palette_to_json(&palette); };
        json_fail_at = -1; json_allocations = 0; char *text = serialize(); ASSERT_NE(text, nullptr);
        int total = json_allocations; json_release(text); EXPECT_EQ(json_live, 0);
        for (int i = 1; i <= total; ++i) {
            json_fail_at = i; json_allocations = 0; text = serialize();
            EXPECT_EQ(text, nullptr) << mode << ":" << i;
            if (text) json_release(text);
            EXPECT_EQ(json_live, 0);
        }
    }
    test_board_display_type = "gc16";
}
TEST_F(ReadPicoSettingsSave, InvalidSerializationInputsRefuseWithoutAllocationOrTruncatedStrings) {
    EXPECT_EQ(processing_settings_to_json(nullptr), nullptr); EXPECT_EQ(color_palette_to_json(nullptr), nullptr);
    for (unsigned field = 0; field < 5; ++field) {
        processing_settings_t settings; processing_settings_get_defaults(&settings);
        char *strings[] = {settings.tone_mode, settings.color_method, settings.dither_algorithm,
                           settings.scale_mode, settings.background_color};
        size_t lengths[] = {sizeof(settings.tone_mode), sizeof(settings.color_method), sizeof(settings.dither_algorithm),
                           sizeof(settings.scale_mode), sizeof(settings.background_color)};
        memset(strings[field], 'x', lengths[field]); EXPECT_EQ(processing_settings_to_json(&settings), nullptr);
    }
    for (unsigned field = 0; field < 7; ++field) {
        processing_settings_t settings; processing_settings_get_defaults(&settings);
        float *numbers[] = {&settings.exposure, &settings.saturation, &settings.contrast, &settings.strength,
                            &settings.shadow_boost, &settings.highlight_compress, &settings.midpoint};
        *numbers[field] = std::numeric_limits<float>::infinity(); EXPECT_EQ(processing_settings_to_json(&settings), nullptr);
    }
    color_palette_t palette; color_palette_get_defaults(&palette); palette.gray_gamma = std::numeric_limits<float>::infinity();
    EXPECT_EQ(color_palette_to_json(&palette), nullptr);
}
TEST_F(ReadPicoSettingsSave, PaletteDefaultBlobBytesAreDeterministicWithoutChangingMeasuredValues) {
    color_palette_t a, b; memset(&a, 0x55, sizeof(a)); memset(&b, 0xaa, sizeof(b));
    color_palette_get_defaults(&a); color_palette_get_defaults(&b);
    EXPECT_EQ(memcmp(&a, &b, sizeof(a)), 0); EXPECT_FLOAT_EQ(a.gray_gamma, 1.42f);
    EXPECT_FLOAT_EQ(a.gray_black_y, .009f); EXPECT_FLOAT_EQ(a.gray_white_y, .65f);
    EXPECT_EQ(a.black.r, 2); EXPECT_EQ(a.white.g, 200);
}
