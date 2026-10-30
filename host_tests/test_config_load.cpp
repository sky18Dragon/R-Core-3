#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>
#include <map>
#include <string>
#include <vector>
extern "C" {
#include "config_manager.h"
#include "nvs.h"
#include "wifi_credentials.h"
}
namespace {
enum Kind { Text, Blob, U8, I32, I64 };
struct Item { Kind kind; std::string text; int64_t integer = 0; };
std::map<std::string, Item> values;
std::map<int, int> handles;
int opens, write_opens, reads, writes, commits, erases, closes, next_handle;
bool namespace_present, persistent = true;
esp_err_t open_error;
esp_err_t save_error, commit_error;
std::string fail_key; esp_err_t fail_error;
std::string fail_write_key;
char *mutate_ssid, *mutate_password, *mutate_name;
wifi_network_t *mutate_network;
int fail_blob_call, blob_calls;
size_t max_allocation = SIZE_MAX;
size_t allocation_calls = 0, fail_allocation = 0;
bool malformed_string;
std::function<void()> ntp_io_hook;
std::function<void()> timezone_env_hook;
int env_error, env_calls, tzset_calls;
bool wrong_environment, force_clock, localtime_failure;
bool strftime_failure;
time_t synthetic_time;
esp_err_t get(nvs_handle_t h, const char *key, Kind kind, Item **item) {
    EXPECT_TRUE(handles.count(h)); ++reads;
    if (key == fail_key) return fail_error;
    auto it = values.find(key);
    if (it == values.end()) return ESP_ERR_NVS_NOT_FOUND;
    if (it->second.kind != kind) return ESP_ERR_NVS_TYPE_MISMATCH;
    *item = &it->second; return ESP_OK;
}
esp_err_t set(nvs_handle_t h, const char *key, Item item) {
    EXPECT_TRUE(handles.count(h)); EXPECT_EQ(handles.at(h), NVS_READWRITE);
    ++writes; if (save_error) return save_error;
    if (key == fail_write_key) return ESP_ERR_NO_MEM;
    values[key] = std::move(item); return ESP_OK;
}
void text(const char *key, const std::string &value) { values[key] = {Text, value}; }
void number(const char *key, Kind kind, int64_t value) { values[key] = {kind, "", value}; }
void legacy_keys() {
    number(NVS_SLEEP_SCHEDULE_ENABLED_KEY, U8, 1);
    text(NVS_SLEEP_SCHEDULE_START_KEY, "22:00"); text(NVS_SLEEP_SCHEDULE_END_KEY, "08:00");
}
class ConfigLoad : public testing::Test {
    std::string timezone; bool had_timezone;
protected:
    void SetUp() override {
        const char *tz = getenv("TZ"); had_timezone = tz != nullptr; timezone = tz ? tz : "";
        namespace_present = true; open_error = save_error = commit_error = 0; fail_key.clear(); fail_error = 0;
        fail_write_key.clear();
        ntp_io_hook = nullptr;
        timezone_env_hook = nullptr; env_error = env_calls = tzset_calls = 0;
        wrong_environment = force_clock = localtime_failure = false; synthetic_time = 1800000000;
        strftime_failure = false;
        mutate_ssid = mutate_password = mutate_name = nullptr; mutate_network = nullptr;
        max_allocation = SIZE_MAX; allocation_calls = fail_allocation = 0;
        malformed_string = false; fail_blob_call = 0; blob_calls = 0;
        config_manager_set_ca_cert_der(nullptr, 0); // Test-owned RAM from a previous synthetic boot.
        values.clear(); handles.clear(); next_handle = 1;
        opens = write_opens = reads = writes = commits = erases = closes = 0; persistent = true;
    }
    void TearDown() override {
        EXPECT_TRUE(handles.empty());
        config_manager_set_ca_cert_der(nullptr, 0);
        if (had_timezone) setenv("TZ", timezone.c_str(), 1); else unsetenv("TZ");
        tzset();
    }
    void read_only() {
        EXPECT_EQ(write_opens, 0);
        EXPECT_EQ(writes, 0); EXPECT_EQ(erases, 0); EXPECT_EQ(commits, 0);
        EXPECT_TRUE(handles.empty());
    }
};
}
extern "C" {
int config_load_test_setenv(const char *name, const char *value, int overwrite) {
    ++env_calls;
    if (timezone_env_hook) { auto hook = std::move(timezone_env_hook); timezone_env_hook = nullptr; hook(); }
    if (env_error) return -1;
    return setenv(name, wrong_environment ? "UTC0" : value, overwrite);
}
void config_load_test_tzset(void) { ++tzset_calls; tzset(); }
time_t config_load_test_time(time_t *out) {
    time_t result = force_clock ? synthetic_time : time(nullptr);
    if (out) *out = result;
    return result;
}
struct tm *config_load_test_localtime_r(const time_t *value, struct tm *out) {
    if (localtime_failure) return nullptr;
    return localtime_r(value, out);
}
size_t config_load_test_strftime(char *out, size_t capacity, const char *format, const struct tm *local) {
    if (strftime_failure) return 0;
    return strftime(out, capacity, format, local);
}
void *config_load_test_malloc(size_t n) {
    ++allocation_calls;
    return n > max_allocation || (fail_allocation && allocation_calls == fail_allocation) ? nullptr : malloc(n);
}
const char *esp_err_to_name(esp_err_t) { return "synthetic error"; }
bool storage_has_persistent_storage(void) { return persistent; }
esp_err_t nvs_open(const char *, int mode, nvs_handle_t *out) {
    ++opens;
    if (mode == NVS_READWRITE) ++write_opens;
    if (ntp_io_hook) {
        auto hook = std::move(ntp_io_hook);
        ntp_io_hook = nullptr;
        hook();
    }
    if (mode == NVS_READWRITE && mutate_network) {
        *mutate_ssid = *mutate_password = *mutate_name = '\0';
        memset(mutate_network, 0, sizeof(*mutate_network));
    }
    if (open_error) return open_error;
    if (!namespace_present && mode == NVS_READONLY) return ESP_ERR_NVS_NOT_FOUND;
    *out = next_handle++; handles[*out] = mode; return ESP_OK;
}
void nvs_close(nvs_handle_t h) { EXPECT_EQ(handles.erase(h), 1u); ++closes; }
esp_err_t nvs_get_str(nvs_handle_t h, const char *key, char *out, size_t *size) {
    Item *item; esp_err_t err = get(h, key, Text, &item); if (err) return err;
    size_t required = item->text.size() + 1, capacity = *size; *size = required;
    if (!out) return ESP_OK;
    if (required > capacity) return ESP_ERR_NVS_INVALID_LENGTH;
    memcpy(out, item->text.c_str(), required);
    if (malformed_string) memset(out, 'x', required); // Emulate a broken storage result.
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *size) {
    if (++blob_calls == fail_blob_call) return ESP_ERR_NVS_NOT_FOUND;
    Item *item; esp_err_t err = get(h, key, Blob, &item); if (err) return err;
    size_t required = item->text.size(), capacity = *size; *size = required;
    if (!out) return ESP_OK;
    if (required > capacity) return ESP_ERR_NVS_INVALID_LENGTH;
    memcpy(out, item->text.data(), required); return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *out) {
    Item *item; auto err = get(h, key, U8, &item); if (!err) *out = item->integer; return err;
}
esp_err_t nvs_get_i32(nvs_handle_t h, const char *key, int32_t *out) {
    Item *item; auto err = get(h, key, I32, &item); if (!err) *out = item->integer; return err;
}
esp_err_t nvs_get_i64(nvs_handle_t h, const char *key, int64_t *out) {
    Item *item; auto err = get(h, key, I64, &item); if (!err) *out = item->integer; return err;
}
esp_err_t nvs_set_str(nvs_handle_t h, const char *key, const char *s) { return set(h, key, {Text, s}); }
esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *p, size_t n) {
    return set(h, key, {Blob, std::string((const char *)p, n)});
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t v) { return set(h, key, {U8, "", v}); }
esp_err_t nvs_set_i32(nvs_handle_t h, const char *key, int32_t v) { return set(h, key, {I32, "", v}); }
esp_err_t nvs_set_i64(nvs_handle_t h, const char *key, int64_t v) { return set(h, key, {I64, "", v}); }
esp_err_t nvs_erase_key(nvs_handle_t h, const char *key) {
    EXPECT_EQ(handles.at(h), NVS_READWRITE); ++erases;
    if (save_error) return save_error;
    if (key == fail_write_key) return ESP_ERR_NO_MEM;
    return values.erase(key) ? ESP_OK : ESP_ERR_NVS_NOT_FOUND;
}
esp_err_t nvs_commit(nvs_handle_t h) { EXPECT_EQ(handles.at(h), NVS_READWRITE); ++commits; return commit_error; }
}

#if CONFIG_BOARD_DRIVER_READ_PICO
namespace {
constexpr uint32_t download_fields = DOWNLOAD_CONFIG_URL | DOWNLOAD_CONFIG_TOKEN |
    DOWNLOAD_CONFIG_HEADER_KEY | DOWNLOAD_CONFIG_HEADER_VALUE | DOWNLOAD_CONFIG_CERTIFICATE;
download_config_t download_patch() {
    download_config_t patch{};
    strcpy(patch.url, "https://synthetic.invalid/image"); strcpy(patch.token, "synthetic-token");
    strcpy(patch.header_key, "X-Synthetic-Key"); strcpy(patch.header_value, "synthetic-value");
    patch.certificate = (uint8_t *)"synthetic-cert"; patch.certificate_length = 14;
    return patch;
}
void expect_download_equal(const download_config_t &a, const download_config_t &b) {
    EXPECT_STREQ(a.url, b.url); EXPECT_STREQ(a.token, b.token);
    EXPECT_STREQ(a.header_key, b.header_key); EXPECT_STREQ(a.header_value, b.header_value);
    EXPECT_STREQ(a.etag, b.etag); ASSERT_EQ(a.certificate_length, b.certificate_length);
    if (a.certificate_length) EXPECT_EQ(memcmp(a.certificate, b.certificate, a.certificate_length), 0);
}
void seed_download() {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    auto patch = download_patch();
    ASSERT_EQ(config_manager_patch_download_checked(download_fields, &patch), ESP_OK);
    strcpy(patch.etag, "\"old\"");
    ASSERT_EQ(config_manager_patch_download_checked(DOWNLOAD_CONFIG_ETAG, &patch), ESP_OK);
}
}
#if CONFIG_READ_PICO_LOCAL_FRAME
TEST_F(ConfigLoad, LocalPolicyMasksOldPreferencesWithoutWritingOrErasingThem) {
    number(NVS_ROTATION_MODE_KEY, U8, ROTATION_MODE_URL);
    number(NVS_DEEP_SLEEP_KEY, U8, 1);
    text(NVS_HA_URL_KEY, "https://example.invalid");
    ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_get_rotation_mode(), ROTATION_MODE_STORAGE);
    EXPECT_FALSE(config_manager_get_deep_sleep_enabled());
    EXPECT_STREQ(config_manager_get_ha_url(), "");
    read_only();
    EXPECT_EQ(values.at(NVS_ROTATION_MODE_KEY).integer, ROTATION_MODE_URL);
    EXPECT_EQ(values.at(NVS_DEEP_SLEEP_KEY).integer, 1);
    EXPECT_EQ(values.at(NVS_HA_URL_KEY).text, "https://example.invalid");
}
#endif

TEST_F(ConfigLoad, DownloadEveryTemporaryAllocationFailurePreservesCacheAndUnlocks) {
    seed_download(); download_config_t before{};
    ASSERT_EQ(config_manager_copy_download(&before), ESP_OK);
    for (int operation = 0; operation < 4; ++operation) {
        const size_t allocations = operation == 0 || operation == 3 ? 3 : 2;
        for (size_t point = 1; point <= allocations; ++point) {
            auto patch = download_patch(); strcpy(patch.token, "changed");
            int old_opens = opens;
            allocation_calls = 0; fail_allocation = point;
            if (operation == 0)
                EXPECT_EQ(config_manager_set_image_source_checked("https://next.invalid", (const uint8_t *)"next", 4), ESP_ERR_NO_MEM);
            else if (operation == 1)
                EXPECT_EQ(config_manager_accept_download_etag(&before, "new"), ESP_ERR_NO_MEM);
            else if (operation == 2)
                EXPECT_EQ(config_manager_patch_download_checked(download_fields, &patch), ESP_ERR_NO_MEM);
            else
                config_manager_set_ca_cert_der((const uint8_t *)"next", 4);
            fail_allocation = 0;
            EXPECT_EQ(opens, old_opens);
            download_config_t after{};
            ASSERT_EQ(config_manager_copy_download(&after), ESP_OK);
            expect_download_equal(before, after);
            config_manager_release_download(&after);
        }
    }
    config_manager_release_download(&before);
}
TEST_F(ConfigLoad, DownloadSnapshotOwnsAllFieldsAndCertificateAcrossReplacementAndClear) {
    seed_download(); download_config_t first{}, second{};
    ASSERT_EQ(config_manager_copy_download(&first), ESP_OK);
    auto patch = download_patch(); expect_download_equal(first, [&] { auto p = patch; strcpy(p.etag, "\"old\""); return p; }());
    EXPECT_NE(first.certificate, config_manager_get_ca_cert_der(nullptr));
    ASSERT_EQ(config_manager_set_image_source_checked("http://other.invalid/image", nullptr, 0), ESP_OK);
    ASSERT_EQ(config_manager_copy_download(&second), ESP_OK);
    EXPECT_STREQ(first.url, "https://synthetic.invalid/image"); EXPECT_EQ(first.certificate_length, 14u);
    EXPECT_EQ(memcmp(first.certificate, "synthetic-cert", 14), 0);
    EXPECT_STREQ(second.url, "http://other.invalid/image"); EXPECT_EQ(second.certificate, nullptr);
    EXPECT_EQ(second.etag[0], 0);
    config_manager_release_download(&first); config_manager_release_download(&first);
    config_manager_release_download(&second); config_manager_release_download(nullptr);
    EXPECT_EQ(first.certificate, nullptr); EXPECT_EQ(first.token[0], 0);
}
TEST_F(ConfigLoad, DownloadSnapshotAllocationFailureLeavesOutputAndCacheUnchanged) {
    seed_download(); download_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
    int old_opens = opens; max_allocation = 0;
    EXPECT_EQ(config_manager_copy_download(&out), ESP_ERR_NO_MEM);
    EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0); EXPECT_EQ(opens, old_opens);
    EXPECT_EQ(config_manager_copy_download(nullptr), ESP_ERR_INVALID_ARG);
    max_allocation = SIZE_MAX; ASSERT_EQ(config_manager_copy_download(&out), ESP_OK);
    EXPECT_STREQ(out.token, "synthetic-token"); config_manager_release_download(&out);
}
TEST_F(ConfigLoad, DownloadPatchValidatesSelectedTextBoundariesAndCertificateBeforeIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); auto patch = download_patch(); int before = opens;
    const uint32_t flags[] = {DOWNLOAD_CONFIG_URL, DOWNLOAD_CONFIG_TOKEN, DOWNLOAD_CONFIG_HEADER_KEY,
                             DOWNLOAD_CONFIG_HEADER_VALUE, DOWNLOAD_CONFIG_ETAG};
    char *texts[] = {patch.url, patch.token, patch.header_key, patch.header_value, patch.etag};
    size_t capacities[] = {sizeof(patch.url), sizeof(patch.token), sizeof(patch.header_key),
                           sizeof(patch.header_value), sizeof(patch.etag)};
    for (size_t i = 0; i < 5; ++i) {
        memset(texts[i], 'x', capacities[i]);
        EXPECT_EQ(config_manager_patch_download_checked(flags[i], &patch), ESP_ERR_INVALID_SIZE);
        EXPECT_EQ(opens, before); texts[i][capacities[i] - 1] = 0;
        EXPECT_EQ(config_manager_patch_download_checked(flags[i], &patch), ESP_OK);
        before = opens;
    }
    patch.certificate_length = CA_CERT_MAX_LEN + 1;
    EXPECT_EQ(config_manager_patch_download_checked(DOWNLOAD_CONFIG_CERTIFICATE, &patch), ESP_ERR_INVALID_SIZE);
    patch.certificate_length = 0;
    EXPECT_EQ(config_manager_patch_download_checked(DOWNLOAD_CONFIG_CERTIFICATE, &patch), ESP_ERR_INVALID_ARG);
    patch.certificate = nullptr; patch.certificate_length = 1;
    EXPECT_EQ(config_manager_patch_download_checked(DOWNLOAD_CONFIG_CERTIFICATE, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_download_checked(0, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_download_checked(1u << 30, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_download_checked(download_fields | DOWNLOAD_CONFIG_ETAG, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_download_checked(DOWNLOAD_CONFIG_URL, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, before);
}
TEST_F(ConfigLoad, DownloadPatchEveryWriteEraseCommitFailureKeepsWholeRamSnapshot) {
    seed_download(); download_config_t before{}; ASSERT_EQ(config_manager_copy_download(&before), ESP_OK);
    auto patch = download_patch(); strcpy(patch.url, "https://next.invalid/image");
    strcpy(patch.token, "new-token"); strcpy(patch.header_key, "X-Next"); strcpy(patch.header_value, "new-value");
    patch.certificate = (uint8_t *)"next-cert"; patch.certificate_length = 9;
    for (const char *key : {NVS_IMAGE_URL_KEY, NVS_ACCESS_TOKEN_KEY, NVS_HTTP_HEADER_KEY_KEY,
                           NVS_HTTP_HEADER_VALUE_KEY, NVS_CA_CERT_KEY, NVS_IMAGE_ETAG_KEY, "commit", "open"}) {
        SCOPED_TRACE(key); fail_write_key = key;
        commit_error = !strcmp(key, "commit") ? ESP_FAIL : ESP_OK;
        open_error = !strcmp(key, "open") ? ESP_ERR_INVALID_STATE : ESP_OK;
        int old_commits = commits;
        EXPECT_NE(config_manager_patch_download_checked(download_fields, &patch), ESP_OK);
        download_config_t after{}; ASSERT_EQ(config_manager_copy_download(&after), ESP_OK);
        expect_download_equal(before, after); config_manager_release_download(&after);
        EXPECT_EQ(commits - old_commits, !strcmp(key, "commit") ? 1 : 0); EXPECT_TRUE(handles.empty());
    }
    fail_write_key.clear(); commit_error = open_error = ESP_OK;
    ASSERT_EQ(config_manager_patch_download_checked(download_fields, &patch), ESP_OK);
    download_config_t after{}; ASSERT_EQ(config_manager_copy_download(&after), ESP_OK);
    EXPECT_STREQ(after.url, patch.url); EXPECT_STREQ(after.token, patch.token); EXPECT_EQ(after.etag[0], 0);
    config_manager_release_download(&after); config_manager_release_download(&before);
}
TEST_F(ConfigLoad, DownloadCertificateAllocationFailureNeverDeletesExistingTrustAnchor) {
    seed_download(); int old_opens = opens; auto old = config_manager_get_ca_cert_der(nullptr);
    max_allocation = 0;
    EXPECT_EQ(config_manager_set_image_source_checked("https://next.invalid", (const uint8_t *)"next", 4), ESP_ERR_NO_MEM);
    EXPECT_EQ(config_manager_get_ca_cert_der(nullptr), old); EXPECT_EQ(opens, old_opens);
    EXPECT_STREQ(config_manager_get_image_url(), "https://synthetic.invalid/image");
    max_allocation = SIZE_MAX;
}
TEST_F(ConfigLoad, DownloadPatchCopiesCallerDataBeforeIoAndRejectsReentry) {
    seed_download(); auto patch = download_patch(); strcpy(patch.url, "https://next.invalid/image");
    uint8_t cert[] = {1, 2, 3, 4}; patch.certificate = cert; patch.certificate_length = sizeof(cert);
    ntp_io_hook = [&] {
        download_config_t out, before; memset(&out, 0xa5, sizeof(out)); before = out;
        EXPECT_EQ(config_manager_copy_download(&out), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
        EXPECT_EQ(config_manager_patch_download_checked(download_fields, &patch), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
        config_manager_set_access_token("nested"); config_manager_set_ca_cert_der(nullptr, 0);
        memset(patch.url, 0, sizeof(patch.url)); memset(patch.token, 0, sizeof(patch.token)); memset(cert, 9, sizeof(cert));
    };
    ASSERT_EQ(config_manager_patch_download_checked(download_fields, &patch), ESP_OK);
    download_config_t out{}; ASSERT_EQ(config_manager_copy_download(&out), ESP_OK);
    EXPECT_STREQ(out.url, "https://next.invalid/image"); EXPECT_STREQ(out.token, "synthetic-token");
    EXPECT_EQ(out.certificate_length, 4u); EXPECT_EQ(out.certificate[0], 1); EXPECT_EQ(out.certificate[3], 4);
    config_manager_release_download(&out);
}
TEST_F(ConfigLoad, DownloadSourceAndCredentialChangesInvalidateEtagButSameValuesKeepIt) {
    for (uint32_t field : {DOWNLOAD_CONFIG_URL, DOWNLOAD_CONFIG_TOKEN, DOWNLOAD_CONFIG_HEADER_KEY,
                           DOWNLOAD_CONFIG_HEADER_VALUE, DOWNLOAD_CONFIG_CERTIFICATE}) {
        seed_download(); auto patch = download_patch();
        EXPECT_EQ(config_manager_patch_download_checked(field, &patch), ESP_OK);
        EXPECT_STREQ(config_manager_get_image_etag(), "\"old\"");
        strcpy(patch.url, "https://next.invalid/image"); strcpy(patch.token, "next");
        strcpy(patch.header_key, "X-Next"); strcpy(patch.header_value, "next");
        patch.certificate = (uint8_t *)"next"; patch.certificate_length = 4;
        EXPECT_EQ(config_manager_patch_download_checked(field, &patch), ESP_OK);
        EXPECT_STREQ(config_manager_get_image_etag(), ""); EXPECT_FALSE(values.count(NVS_IMAGE_ETAG_KEY));
    }
}
TEST_F(ConfigLoad, DownloadCompletedOldRequestCannotRepopulateEtagAfterSourceOrCredentialChange) {
    for (uint32_t field : {DOWNLOAD_CONFIG_URL, DOWNLOAD_CONFIG_TOKEN, DOWNLOAD_CONFIG_HEADER_KEY,
                           DOWNLOAD_CONFIG_HEADER_VALUE, DOWNLOAD_CONFIG_CERTIFICATE}) {
        seed_download(); download_config_t request{}; ASSERT_EQ(config_manager_copy_download(&request), ESP_OK);
        auto patch = download_patch(); strcpy(patch.url, "https://next.invalid/image"); strcpy(patch.token, "next");
        strcpy(patch.header_key, "X-Next"); strcpy(patch.header_value, "next");
        patch.certificate = nullptr; patch.certificate_length = 0;
        ASSERT_EQ(config_manager_patch_download_checked(field, &patch), ESP_OK);
        int old_opens = opens;
        EXPECT_EQ(config_manager_accept_download_etag(&request, "\"stale\""), ESP_OK);
        EXPECT_EQ(opens, old_opens); EXPECT_STREQ(config_manager_get_image_etag(), "");
        config_manager_release_download(&request);
    }
}
TEST_F(ConfigLoad, DownloadEtagSaveErrorsKeepPriorCacheAndRetryChecksSameRequestIdentity) {
    seed_download(); download_config_t request{}; ASSERT_EQ(config_manager_copy_download(&request), ESP_OK);
    commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_accept_download_etag(&request, "\"next\""), ESP_FAIL);
    EXPECT_STREQ(config_manager_get_image_etag(), "\"old\"");
    commit_error = ESP_OK;
    EXPECT_EQ(config_manager_accept_download_etag(&request, "\"next\""), ESP_OK);
    int old_opens = opens;
    EXPECT_EQ(config_manager_accept_download_etag(&request, "\"next\""), ESP_OK);
    EXPECT_EQ(opens, old_opens);
    std::string huge(HTTP_ETAG_MAX_LEN, 'x');
    EXPECT_EQ(config_manager_accept_download_etag(&request, huge.c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_manager_accept_download_etag(nullptr, ""), ESP_ERR_INVALID_ARG);
    config_manager_release_download(&request);
}
TEST_F(ConfigLoad, DownloadClearMissingRecordsIsAllowedButEraseFailureRetainsSource) {
    seed_download(); auto old = config_manager_get_ca_cert_der(nullptr);
    fail_write_key = NVS_CA_CERT_KEY;
    EXPECT_EQ(config_manager_set_image_source_checked("", nullptr, 0), ESP_ERR_NO_MEM);
    EXPECT_EQ(config_manager_get_ca_cert_der(nullptr), old);
    EXPECT_STREQ(config_manager_get_image_url(), "https://synthetic.invalid/image");
    fail_write_key.clear(); EXPECT_EQ(config_manager_set_image_source_checked("", nullptr, 0), ESP_OK);
    EXPECT_EQ(config_manager_set_image_source_checked("", nullptr, 0), ESP_OK);
    EXPECT_EQ(config_manager_get_ca_cert_der(nullptr), nullptr); EXPECT_STREQ(config_manager_get_image_url(), "");
}
TEST_F(ConfigLoad, DownloadCompatibilityWritersDoNotPublishOnFailureOrTruncate) {
    seed_download(); save_error = ESP_FAIL;
    config_manager_set_image_url("http://next.invalid"); config_manager_set_access_token("next");
    config_manager_set_http_header_key("X-Next"); config_manager_set_http_header_value("next");
    config_manager_set_ca_cert_der(nullptr, 0); config_manager_set_image_etag("\"next\"");
    EXPECT_STREQ(config_manager_get_image_url(), "https://synthetic.invalid/image");
    EXPECT_STREQ(config_manager_get_access_token(), "synthetic-token");
    EXPECT_STREQ(config_manager_get_http_header_key(), "X-Synthetic-Key");
    EXPECT_STREQ(config_manager_get_http_header_value(), "synthetic-value");
    EXPECT_STREQ(config_manager_get_image_etag(), "\"old\"");
    save_error = ESP_OK; int old_opens = opens;
    std::string huge(ACCESS_TOKEN_MAX_LEN, 'x'); config_manager_set_access_token(huge.c_str());
    EXPECT_EQ(opens, old_opens); EXPECT_STREQ(config_manager_get_access_token(), "synthetic-token");
}
namespace {
struct NameEffects {
    int mdns_calls = 0, wifi_calls = 0;
    esp_err_t mdns_error = ESP_OK, wifi_error = ESP_OK;
    std::string mdns_name, wifi_name;
    std::function<void()> hook;
    static esp_err_t mdns(void *context, const char *name) {
        auto *state = static_cast<NameEffects *>(context);
        ++state->mdns_calls; state->mdns_name = name;
        if (state->hook) state->hook();
        return state->mdns_error;
    }
    static esp_err_t wifi(void *context, const char *name) {
        auto *state = static_cast<NameEffects *>(context);
        ++state->wifi_calls; state->wifi_name = name; return state->wifi_error;
    }
    device_name_ops_t ops() { return {this, mdns, wifi}; }
};
}
TEST_F(ConfigLoad, NameApplySaveThenBothEffectsConfirmOnce) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_TRUE(result.name_accepted); EXPECT_TRUE(result.hostnames_confirmed); EXPECT_EQ(result.step, DEVICE_NAME_COMPLETE);
    EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1);
    EXPECT_EQ(effects.mdns_name, "new"); EXPECT_EQ(effects.wifi_name, "new");
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_EQ(effects.mdns_calls, 1); EXPECT_EQ(effects.wifi_calls, 1); EXPECT_EQ(writes, 1);
}
TEST_F(ConfigLoad, NameApplyMdnsFailureRetriesSameValueWithoutRewritingNvs) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    effects.mdns_error = ESP_FAIL;
    EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_FAIL);
    EXPECT_TRUE(result.name_accepted); EXPECT_FALSE(result.hostnames_confirmed); EXPECT_EQ(result.step, DEVICE_NAME_MDNS);
    EXPECT_EQ(effects.wifi_calls, 0); EXPECT_STREQ(config_manager_get_device_name(), "new");
    effects.mdns_error = ESP_OK;
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1); EXPECT_EQ(effects.mdns_calls, 2); EXPECT_EQ(effects.wifi_calls, 1);
}
TEST_F(ConfigLoad, NameApplyWifiFailureRetriesBothEffectsAndKeepsName) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    effects.wifi_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(result.step, DEVICE_NAME_WIFI); EXPECT_TRUE(result.name_accepted); EXPECT_FALSE(result.hostnames_confirmed);
    effects.wifi_error = ESP_OK;
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_EQ(writes, 1); EXPECT_EQ(effects.mdns_calls, 2); EXPECT_EQ(effects.wifi_calls, 2);
}
TEST_F(ConfigLoad, NameApplyUnchangedLoadedValueNeedsEffectsButNoNvsWrite) {
    text(NVS_DEVICE_NAME_KEY, "loaded"); ASSERT_EQ(config_manager_init(), ESP_OK);
    NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    EXPECT_EQ(config_manager_apply_device_name_checked("loaded", &ops, &result), ESP_OK);
    EXPECT_TRUE(result.hostnames_confirmed); EXPECT_EQ(writes, 0); EXPECT_EQ(effects.mdns_calls, 1);
}
TEST_F(ConfigLoad, NameApplyEverySaveFailureStopsBeforeEffectsAndKeepsCurrentName) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    for (int step = 0; step < 3; ++step) {
        open_error = step == 0 ? ESP_FAIL : ESP_OK;
        save_error = step == 1 ? ESP_FAIL : ESP_OK;
        commit_error = step == 2 ? ESP_FAIL : ESP_OK;
        EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_FAIL);
        EXPECT_FALSE(result.name_accepted); EXPECT_EQ(result.step, DEVICE_NAME_SAVE);
        EXPECT_EQ(effects.mdns_calls, 0); EXPECT_EQ(effects.wifi_calls, 0);
        EXPECT_STREQ(config_manager_get_device_name(), DEFAULT_DEVICE_NAME);
    }
    EXPECT_TRUE(handles.empty());
}
TEST_F(ConfigLoad, NameApplyInputMutationDoesNotChangeEitherCallbackName) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    char name[64] = "candidate";
    ntp_io_hook = [&] { strcpy(name, "mutated"); };
    effects.hook = [&] { strcpy(name, "changed-again"); };
    EXPECT_EQ(config_manager_apply_device_name_checked(name, &ops, &result), ESP_OK);
    EXPECT_EQ(effects.mdns_name, "candidate"); EXPECT_EQ(effects.wifi_name, "candidate");
    EXPECT_STREQ(config_manager_get_device_name(), "candidate");
}
TEST_F(ConfigLoad, NameApplyHoldsConfigurationControlAcrossBothEffects) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    effects.hook = [&] {
        char out[64] = "unchanged"; wifi_network_t network{}; device_name_result_t nested{true, true, DEVICE_NAME_COMPLETE};
        EXPECT_EQ(config_manager_copy_device_name(out, sizeof(out)), ESP_ERR_INVALID_STATE); EXPECT_STREQ(out, "unchanged");
        EXPECT_EQ(config_manager_set_device_name_checked("nested"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_apply_device_name_checked("nested", &ops, &nested), ESP_ERR_INVALID_STATE);
        EXPECT_TRUE(nested.hostnames_confirmed);
        EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "nested", &network), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
    };
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK); EXPECT_EQ(writes, 1);
}
TEST_F(ConfigLoad, NameApplyInvalidInputsDoNotTouchStorageOrCallbacks) {
    NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    for (const char *name : {"", "bad\nname"}) EXPECT_EQ(config_manager_apply_device_name_checked(name, &ops, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_apply_device_name_checked(nullptr, &ops, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_apply_device_name_checked(std::string(64, 'x').c_str(), &ops, &result), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_manager_apply_device_name_checked("new", nullptr, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, nullptr), ESP_ERR_INVALID_ARG);
    ops.mdns = nullptr; EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, 0); EXPECT_EQ(effects.mdns_calls, 0);
}
TEST_F(ConfigLoad, NameApplyExplicitSetterInvalidatesSameNameConfirmation) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    ASSERT_EQ(config_manager_set_device_name_checked("new"), ESP_OK);
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_EQ(effects.mdns_calls, 2); EXPECT_EQ(writes, 2);
}
TEST_F(ConfigLoad, NameApplyProvisioningInvalidatesConfirmationAndRetriesExistingName) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    wifi_network_t network{};
    ASSERT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_OK);
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    EXPECT_EQ(effects.mdns_calls, 2); EXPECT_EQ(writes, 9);
}
TEST_F(ConfigLoad, NameApplyFailedReloadRetainsConfirmationButSuccessfulReloadRequiresEffects) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    ASSERT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK);
    fail_key = NVS_TIMEZONE_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL);
    EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK); EXPECT_EQ(effects.mdns_calls, 1);
    fail_key.clear(); ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_apply_device_name_checked("new", &ops, &result), ESP_OK); EXPECT_EQ(effects.mdns_calls, 2);
}
TEST_F(ConfigLoad, NameApplyActualConcurrentWriterCannotReplaceNameDuringEffects) {
    ASSERT_EQ(config_manager_init(), ESP_OK); NameEffects effects; auto ops = effects.ops(); device_name_result_t result{};
    std::atomic<bool> entered{false}, release{false};
    effects.hook = [&] {
        entered = true; auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!release.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    };
    esp_err_t applied = ESP_FAIL;
    std::thread worker([&] { applied = config_manager_apply_device_name_checked("new", &ops, &result); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!entered.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_TRUE(entered.load()); EXPECT_EQ(config_manager_set_device_name_checked("newer"), ESP_ERR_INVALID_STATE);
    release = true; worker.join(); EXPECT_EQ(applied, ESP_OK); EXPECT_EQ(effects.wifi_name, "new");
    EXPECT_STREQ(config_manager_get_device_name(), "new");
}
TEST_F(ConfigLoad, NameSnapshotOwnedAndExactCapacityChecked) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    char out[64]; ASSERT_EQ(config_manager_copy_device_name(out, sizeof(out)), ESP_OK);
    EXPECT_STREQ(out, DEFAULT_DEVICE_NAME);
    ASSERT_EQ(config_manager_set_device_name_checked("new"), ESP_OK);
    EXPECT_STREQ(out, DEFAULT_DEVICE_NAME);
    char small[4] = "old";
    EXPECT_EQ(config_manager_copy_device_name(small, 3), ESP_ERR_INVALID_SIZE); EXPECT_STREQ(small, "old");
    EXPECT_EQ(config_manager_copy_device_name(small, 4), ESP_OK); EXPECT_STREQ(small, "new");
    EXPECT_EQ(config_manager_copy_device_name(nullptr, 64), ESP_ERR_INVALID_ARG);
}
TEST_F(ConfigLoad, NameShapeRejectsControlAndTruncationBeforeIo) {
    for (const char *name : {"", "a\nb", "a\tb", "a\x7f"})
        EXPECT_EQ(config_manager_set_device_name_checked(name), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_set_device_name_checked(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_set_device_name_checked(std::string(64, 'x').c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(opens, 0);
    EXPECT_EQ(config_manager_set_device_name_checked(std::string(63, 'x').c_str()), ESP_OK);
    char out[64]; ASSERT_EQ(config_manager_copy_device_name(out, sizeof(out)), ESP_OK); EXPECT_EQ(strlen(out), 63u);
}
TEST_F(ConfigLoad, NameSnapshotsInputBeforeIoAndRejectsReentry) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char name[64] = "candidate";
    ntp_io_hook = [&] {
        strcpy(name, "mutated"); char out[64] = "unchanged"; wifi_network_t network{};
        EXPECT_EQ(config_manager_copy_device_name(out, sizeof(out)), ESP_ERR_INVALID_STATE);
        EXPECT_STREQ(out, "unchanged");
        EXPECT_EQ(config_manager_set_device_name_checked("nested"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_patch_network_checked(0, &network), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
    };
    ASSERT_EQ(config_manager_set_device_name_checked(name), ESP_OK);
    EXPECT_EQ(values.at(NVS_DEVICE_NAME_KEY).text, "candidate");
    EXPECT_STREQ(config_manager_get_device_name(), "candidate"); EXPECT_EQ(write_opens, 1);
}
TEST_F(ConfigLoad, NameVoidCompatibilityDoesNotPublishFailedSave) {
    ASSERT_EQ(config_manager_init(), ESP_OK); commit_error = ESP_FAIL;
    config_manager_set_device_name("candidate");
    EXPECT_STREQ(config_manager_get_device_name(), DEFAULT_DEVICE_NAME);
    EXPECT_EQ(values.at(NVS_DEVICE_NAME_KEY).text, "candidate"); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, NameMalformedLoadRetainsConfirmedCacheWithoutWrites) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ASSERT_EQ(config_manager_set_device_name_checked("confirmed"), ESP_OK);
    text(NVS_DEVICE_NAME_KEY, "invalid\nname"); int before = writes;
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE);
    char out[64]; ASSERT_EQ(config_manager_copy_device_name(out, sizeof(out)), ESP_OK);
    EXPECT_STREQ(out, "confirmed"); EXPECT_EQ(writes, before);
}
TEST_F(ConfigLoad, NetworkFullPatchUsesFiveSetsOneCommitAndOwnedSnapshot) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{};
    ASSERT_TRUE(wifi_network_copy(&patch, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", "8.8.8.8"));
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_ALL, &patch), ESP_OK);
    EXPECT_EQ(writes, 5); EXPECT_EQ(commits, 1); EXPECT_EQ(write_opens, 1); EXPECT_TRUE(handles.empty());
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_TRUE(out.static_address); EXPECT_STREQ(out.ip, "1.2.3.4");
    strcpy(patch.dns, "1.1.1.1");
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_OK);
    EXPECT_STREQ(out.dns, "8.8.8.8");
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.ip, "1.2.3.4"); EXPECT_STREQ(out.dns, "1.1.1.1");
}
TEST_F(ConfigLoad, NetworkOpenFailureHasNoWritesOrRamPublication) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{}; strcpy(patch.dns, "8.8.8.8");
    open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_FAIL);
    EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0); EXPECT_EQ(closes, 1);
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "");
}
TEST_F(ConfigLoad, NetworkEverySetFailureStopsAndRetainsWholeRamGroup) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{};
    ASSERT_TRUE(wifi_network_copy(&patch, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", "8.8.8.8"));
    const char *keys[] = {NVS_STATIC_IP_KEY, NVS_STATIC_NETMASK_KEY, NVS_STATIC_GATEWAY_KEY, NVS_DNS_SERVER_KEY, NVS_IP_MODE_KEY};
    for (int i = 0; i < 5; ++i) {
        fail_write_key = keys[i]; int before = writes;
        EXPECT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_ALL, &patch), ESP_ERR_NO_MEM);
        EXPECT_EQ(writes - before, i + 1); EXPECT_EQ(commits, 0); EXPECT_TRUE(handles.empty());
        ASSERT_EQ(config_manager_copy_network(&out), ESP_OK);
        EXPECT_FALSE(out.static_address); EXPECT_STREQ(out.ip, ""); EXPECT_STREQ(out.dns, "");
    }
}
TEST_F(ConfigLoad, NetworkCommitFailureDoesNotClaimPersistentRollback) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{}; strcpy(patch.dns, "8.8.8.8");
    commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_FAIL);
    EXPECT_EQ(values.at(NVS_DNS_SERVER_KEY).text, "8.8.8.8");
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "");
    commit_error = ESP_OK;
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_OK);
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "8.8.8.8");
}
TEST_F(ConfigLoad, NetworkInvalidPartialStaticAndEmptyFieldsHaveNoIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}; patch.static_address = true;
    int before = opens;
    EXPECT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_MODE, &patch), ESP_ERR_INVALID_ARG);
    strcpy(patch.ip, "bad");
    EXPECT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_IP, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_network_checked(32, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_patch_network_checked(0, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_network(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, before); EXPECT_EQ(writes, 0);
}
TEST_F(ConfigLoad, NetworkNoFieldsDoesNotPersistAndDhcpCanClearWholeStaticSet) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{};
    EXPECT_EQ(config_manager_patch_network_checked(0, &patch), ESP_OK); EXPECT_EQ(writes, 0);
    ASSERT_TRUE(wifi_network_copy(&patch, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", ""));
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_ALL, &patch), ESP_OK);
    memset(&patch, 0, sizeof(patch));
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_ALL, &patch), ESP_OK);
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_FALSE(out.static_address); EXPECT_STREQ(out.ip, "");
}
TEST_F(ConfigLoad, NetworkInputSnapshotAndBusyCopiesDuringIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{}; strcpy(patch.dns, "8.8.8.8"); strcpy(out.ip, "unchanged");
    ntp_io_hook = [&] {
        memset(&patch, 0, sizeof(patch));
        EXPECT_EQ(config_manager_copy_network(&out), ESP_ERR_INVALID_STATE); EXPECT_STREQ(out.ip, "unchanged");
        EXPECT_EQ(config_manager_set_ntp_server_checked("example.test"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_set_device_name_checked("nested"), ESP_ERR_INVALID_STATE);
        wifi_network_t candidate{};
        EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "nested", &candidate), ESP_ERR_INVALID_STATE);
    };
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_OK);
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "8.8.8.8"); EXPECT_EQ(write_opens, 1);
}
TEST_F(ConfigLoad, ProvisioningHoldsSameControlForNameNetworkAndReload) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t network{}, out{}; char name[64] = "unchanged";
    ntp_io_hook = [&] {
        EXPECT_EQ(config_manager_copy_network(&out), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_device_name(name, sizeof(name)), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_patch_network_checked(0, &network), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_set_device_name_checked("nested"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
    };
    ASSERT_EQ(config_manager_save_provisioning_checked("network", "password", "candidate", &network), ESP_OK);
    EXPECT_STREQ(name, "unchanged"); EXPECT_EQ(write_opens, 1);
    ASSERT_EQ(config_manager_copy_device_name(name, sizeof(name)), ESP_OK); EXPECT_STREQ(name, "candidate");
}
TEST_F(ConfigLoad, NetworkVoidCompatibilityCannotPublishInvalidOrFailedCandidate) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t out{};
    config_manager_set_static_ip("bad"); config_manager_set_ip_mode((ip_mode_t)99);
    EXPECT_EQ(writes, 0);
    commit_error = ESP_FAIL; config_manager_set_dns_server("8.8.8.8");
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "");
}
TEST_F(ConfigLoad, FailedReloadRetainsConfirmedNameAndNetworkGroup) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{}; strcpy(patch.dns, "8.8.8.8");
    ASSERT_EQ(config_manager_set_device_name_checked("confirmed"), ESP_OK);
    ASSERT_EQ(config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch), ESP_OK);
    text(NVS_DEVICE_NAME_KEY, "unconfirmed"); text(NVS_DNS_SERVER_KEY, "1.1.1.1");
    fail_key = NVS_TIMEZONE_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL);
    char name[64]; ASSERT_EQ(config_manager_copy_device_name(name, sizeof(name)), ESP_OK); EXPECT_STREQ(name, "confirmed");
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "8.8.8.8");
}
TEST_F(ConfigLoad, NetworkActualConcurrentReaderCannotObserveUnconfirmedWrite) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t patch{}, out{}; strcpy(patch.dns, "8.8.8.8"); strcpy(out.ip, "unchanged");
    std::atomic<bool> entered{false}, release{false};
    ntp_io_hook = [&] {
        entered = true;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!release.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    };
    esp_err_t saved = ESP_FAIL;
    std::thread writer([&] { saved = config_manager_patch_network_checked(WIFI_NETWORK_PATCH_DNS, &patch); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!entered.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_TRUE(entered.load());
    EXPECT_EQ(config_manager_copy_network(&out), ESP_ERR_INVALID_STATE); EXPECT_STREQ(out.ip, "unchanged");
    EXPECT_EQ(config_manager_set_device_name_checked("nested"), ESP_ERR_INVALID_STATE);
    release = true; writer.join(); ASSERT_EQ(saved, ESP_OK);
    ASSERT_EQ(config_manager_copy_network(&out), ESP_OK); EXPECT_STREQ(out.dns, "8.8.8.8");
}
TEST_F(ConfigLoad, RevisionCopyBeforeLoadIsZeroWithoutStorageIo) {
    int64_t out = 42;
    EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 0);
    EXPECT_EQ(config_manager_copy_config_last_updated(nullptr), ESP_ERR_INVALID_ARG); EXPECT_EQ(opens, 0);
}
TEST_F(ConfigLoad, RevisionTouchPublishesOnlyAfterOneSetCommitAndClose) {
    force_clock = true; synthetic_time = 1800000000;
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_OK);
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK);
    EXPECT_EQ(out, 1800000000); EXPECT_EQ(values.at("cfg_updated").integer, out);
    EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1); EXPECT_EQ(closes, 1);
}
TEST_F(ConfigLoad, RevisionRepeatedAndBackwardsClockStillAdvancesWithoutOverflow) {
    force_clock = true; synthetic_time = 1800000000;
    ASSERT_EQ(config_manager_touch_config_checked(), ESP_OK);
    ASSERT_EQ(config_manager_touch_config_checked(), ESP_OK);
    synthetic_time = 1577836800; ASSERT_EQ(config_manager_touch_config_checked(), ESP_OK);
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 1800000002);
}
TEST_F(ConfigLoad, RevisionInvalidCurrentClockDoesNotWriteOrPublish) {
    force_clock = true;
    for (time_t invalid : {(time_t)-1, (time_t)0, (time_t)1577836799}) {
        synthetic_time = invalid; EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_INVALID_STATE);
    }
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 0); EXPECT_EQ(opens, 0);
}
TEST_F(ConfigLoad, RevisionOpenAndSetFailureDoNotAdvanceRamOrCommit) {
    force_clock = true; open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_FAIL); EXPECT_EQ(writes, 0); EXPECT_EQ(closes, 0);
    open_error = ESP_OK; save_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_NO_MEM); EXPECT_EQ(commits, 0); EXPECT_EQ(closes, 1);
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 0);
}
TEST_F(ConfigLoad, RevisionCommitFailureMayPersistButNeverPublishesConfirmedRam) {
    force_clock = true; commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_FAIL);
    EXPECT_EQ(values.at("cfg_updated").integer, synthetic_time);
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 0);
    commit_error = ESP_OK; EXPECT_EQ(config_manager_touch_config_checked(), ESP_OK);
    EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, synthetic_time);
}
TEST_F(ConfigLoad, RevisionReentryCannotTouchCopyDirectSetOrReloadDuringIo) {
    force_clock = true; ntp_io_hook = [] {
        int64_t out = 42;
        EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
        config_manager_set_config_last_updated(1900000000);
        EXPECT_EQ(out, 42);
    };
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_OK); EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, RevisionTouchSharesTimezoneControlWithoutPublishingNestedValue) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ntp_io_hook = [] { EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_INVALID_STATE); };
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK); EXPECT_FALSE(values.count("cfg_updated"));
}
TEST_F(ConfigLoad, RevisionMaximumStoredValueRefusesIncrementWithoutWrite) {
    number("cfg_updated", I64, INT64_MAX); ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_INVALID_STATE); read_only();
    int64_t out = 0; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, INT64_MAX);
}
TEST_F(ConfigLoad, RevisionFailedReloadDoesNotPublishUnconfirmedLoadedTimestamp) {
    number("cfg_updated", I64, 1800000000); ASSERT_EQ(config_manager_init(), ESP_OK);
    number("cfg_updated", I64, 1900000000); fail_key = NVS_AUTO_ROTATE_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL);
    int64_t out = 42; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 1800000000);
    read_only();
}
TEST_F(ConfigLoad, RevisionActualConcurrentReaderCannotObserveAnUnconfirmedWrite) {
    force_clock = true; std::atomic<int> stage{0}; esp_err_t result = ESP_FAIL;
    ntp_io_hook = [&] {
        stage.store(1); auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (stage.load() != 2 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    };
    std::thread writer([&] { result = config_manager_touch_config_checked(); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (stage.load() != 1 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_EQ(stage.load(), 1); int64_t out = 42;
    EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_ERR_INVALID_STATE); EXPECT_EQ(out, 42);
    EXPECT_EQ(config_manager_touch_config_checked(), ESP_ERR_INVALID_STATE);
    stage.store(2); writer.join(); EXPECT_EQ(result, ESP_OK);
    EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, synthetic_time); EXPECT_EQ(writes, 1);
}
TEST_F(ConfigLoad, RevisionDirectVoidSetAlsoChecksPersistenceAndRefusesRegression) {
    config_manager_set_config_last_updated(1800000000); EXPECT_EQ(writes, 1);
    config_manager_set_config_last_updated(1); config_manager_set_config_last_updated(1799999999); EXPECT_EQ(writes, 1);
    commit_error = ESP_FAIL; config_manager_set_config_last_updated(1900000000);
    int64_t out = 0; EXPECT_EQ(config_manager_copy_config_last_updated(&out), ESP_OK); EXPECT_EQ(out, 1800000000);
    EXPECT_EQ(values.at("cfg_updated").integer, 1900000000); // No persistence rollback claim.
}
TEST_F(ConfigLoad, TimezoneCopyBeforeLoadReportsUnappliedDefaultWithoutIo) {
    char timezone[64]; bool applied = true;
    EXPECT_EQ(config_manager_copy_timezone(timezone, sizeof(timezone), &applied), ESP_OK);
    EXPECT_STREQ(timezone, DEFAULT_TIMEZONE); EXPECT_FALSE(applied); read_only();
    time_t stamp = 42; char local[64] = "unchanged";
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), timezone, sizeof(timezone)), ESP_ERR_NOT_FINISHED);
    EXPECT_EQ(stamp, 42);
}
TEST_F(ConfigLoad, TimezoneSaveChecksCommitCloseBeforeApplyingAndPublishing) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = closes;
    timezone_env_hook = [&] { EXPECT_EQ(closes, before + 1); EXPECT_EQ(commits, 1); };
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1); EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8");
    EXPECT_STREQ(getenv("TZ"), "CST-8"); EXPECT_STREQ(config_manager_get_timezone(), "CST-8");
    char timezone[64]; bool applied = false;
    EXPECT_EQ(config_manager_copy_timezone(timezone, sizeof(timezone), &applied), ESP_OK); EXPECT_TRUE(applied);
}
TEST_F(ConfigLoad, TimezoneOpenFailureDoesNotPublishOrApply) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int env_before = env_calls, tz_before = tzset_calls;
    open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_FAIL);
    EXPECT_STREQ(config_manager_get_timezone(), DEFAULT_TIMEZONE); EXPECT_STREQ(getenv("TZ"), DEFAULT_TIMEZONE);
    EXPECT_EQ(env_calls, env_before); EXPECT_EQ(tzset_calls, tz_before); EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ConfigLoad, TimezoneSetFailureClosesWithoutCommitRamOrEnvironmentChange) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = env_calls; save_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_ERR_NO_MEM);
    EXPECT_EQ(commits, 0); EXPECT_TRUE(handles.empty()); EXPECT_EQ(env_calls, before);
    EXPECT_STREQ(config_manager_get_timezone(), DEFAULT_TIMEZONE);
}
TEST_F(ConfigLoad, TimezoneCommitFailureDoesNotClaimStorageRollbackOrApply) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = env_calls; commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_FAIL);
    EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8"); EXPECT_EQ(env_calls, before);
    EXPECT_STREQ(config_manager_get_timezone(), DEFAULT_TIMEZONE); EXPECT_STREQ(getenv("TZ"), DEFAULT_TIMEZONE);
}
TEST_F(ConfigLoad, TimezoneEnvironmentFailureReportsSavedButUnappliedAndExplicitRetryWorks) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = tzset_calls; env_error = 1;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_ERR_NOT_FINISHED);
    EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8"); EXPECT_STREQ(config_manager_get_timezone(), "CST-8");
    EXPECT_EQ(tzset_calls, before); EXPECT_STREQ(getenv("TZ"), DEFAULT_TIMEZONE);
    char timezone[64]; bool applied = true;
    EXPECT_EQ(config_manager_copy_timezone(timezone, sizeof(timezone), &applied), ESP_OK); EXPECT_FALSE(applied);
    time_t stamp = 42; char local[64] = "unchanged";
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), timezone, sizeof(timezone)), ESP_ERR_NOT_FINISHED);
    EXPECT_EQ(stamp, 42);
    env_error = 0; EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    EXPECT_EQ(config_manager_copy_timezone(timezone, sizeof(timezone), &applied), ESP_OK); EXPECT_TRUE(applied);
}
TEST_F(ConfigLoad, TimezoneInitEnvironmentFailureRefusesStartupWithoutWriting) {
    env_error = 1;
    EXPECT_EQ(config_manager_init(), ESP_ERR_NOT_FINISHED); EXPECT_EQ(env_calls, 1); EXPECT_EQ(tzset_calls, 0); read_only();
}
TEST_F(ConfigLoad, TimezoneEnvironmentReadbackMismatchIsNotReportedApplied) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wrong_environment = true;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_ERR_NOT_FINISHED);
    char timezone[64]; bool applied = true;
    EXPECT_EQ(config_manager_copy_timezone(timezone, sizeof(timezone), &applied), ESP_OK); EXPECT_FALSE(applied);
    EXPECT_STREQ(timezone, "CST-8"); EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8");
}
TEST_F(ConfigLoad, TimezoneInputSnapshotPrecedesIoMutation) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char value[] = "CST-8";
    ntp_io_hook = [&] { memset(value, 'x', sizeof(value) - 1); };
    EXPECT_EQ(config_manager_set_timezone(value), ESP_OK);
    EXPECT_STREQ(config_manager_get_timezone(), "CST-8"); EXPECT_STREQ(getenv("TZ"), "CST-8");
    EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8");
}
TEST_F(ConfigLoad, TimezoneNullEmptyControlAndNonAsciiRejectedBeforeIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = opens, env_before = env_calls;
    for (auto value : {"", "UTC0\n", "UTC\x7f", "UTC\xc2\xa0"})
        EXPECT_EQ(config_manager_set_timezone(value), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_set_timezone(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, before); EXPECT_EQ(env_calls, env_before); EXPECT_EQ(writes, 0);
}
TEST_F(ConfigLoad, TimezoneMaximumLengthNotTruncatedAndOverlongRejected) {
    ASSERT_EQ(config_manager_init(), ESP_OK); std::string longest = "UTC0" + std::string(59, ' ');
    EXPECT_EQ(config_manager_set_timezone(longest.c_str()), ESP_OK); EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, longest);
    int before = opens;
    EXPECT_EQ(config_manager_set_timezone((longest + " ").c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(opens, before); EXPECT_EQ(std::string(config_manager_get_timezone()), longest);
}
TEST_F(ConfigLoad, TimezoneSnapshotIsCallerOwnedAndChecksExactCapacity) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char out[64] = "unchanged"; bool applied = false;
    EXPECT_EQ(config_manager_copy_timezone(out, 4, &applied), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(out, "unchanged"); EXPECT_FALSE(applied);
    EXPECT_EQ(config_manager_copy_timezone(out, 5, &applied), ESP_OK); EXPECT_STREQ(out, "UTC0"); EXPECT_TRUE(applied);
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK); EXPECT_STREQ(out, "UTC0");
}
TEST_F(ConfigLoad, TimezoneStorageReentryRejectsNtpInitTimeAndCopiesWithoutExtraIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ntp_io_hook = [] {
        char out[128] = "unchanged", local[64] = "unchanged"; bool applied = false; time_t stamp = 42;
        EXPECT_EQ(config_manager_set_timezone("UTC1"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_set_ntp_server_checked("clock.example"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_ntp_server(out, sizeof(out)), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), out, sizeof(out)), ESP_ERR_INVALID_STATE);
        EXPECT_STREQ(out, "unchanged"); EXPECT_EQ(stamp, 42); EXPECT_FALSE(applied);
    };
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK); EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, TimezoneApplicationHoldsControlThroughEnvironmentAndTzset) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    timezone_env_hook = [] {
        char out[64] = "unchanged"; bool applied = false;
        EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_set_timezone("UTC1"), ESP_ERR_INVALID_STATE);
        EXPECT_STREQ(out, "unchanged");
    };
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK); EXPECT_EQ(writes, 1);
}
TEST_F(ConfigLoad, TimezoneActualConcurrentStorageDoesNotPublishMixedTimeConfiguration) {
    ASSERT_EQ(config_manager_init(), ESP_OK); std::atomic<int> stage{0}; esp_err_t result = ESP_FAIL;
    ntp_io_hook = [&] {
        stage.store(1); auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (stage.load() != 2 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    };
    std::thread writer([&] { result = config_manager_set_timezone("CST-8"); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (stage.load() != 1 && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_EQ(stage.load(), 1); char out[64] = "unchanged"; bool applied = false;
    EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(config_manager_set_ntp_server_checked("clock.example"), ESP_ERR_INVALID_STATE);
    stage.store(2); writer.join(); EXPECT_EQ(result, ESP_OK); EXPECT_STREQ(out, "unchanged");
    EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_OK); EXPECT_TRUE(applied); EXPECT_STREQ(out, "CST-8");
}
TEST_F(ConfigLoad, TimezoneLocalTimeInvalidClockDoesNotChangeAnyOutput) {
    ASSERT_EQ(config_manager_init(), ESP_OK); force_clock = true;
    char out[64] = "unchanged", local[64] = "unchanged"; time_t stamp = 42;
    for (time_t invalid : {(time_t)-1, (time_t)0, (time_t)1577836799}) {
        synthetic_time = invalid;
        EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), out, sizeof(out)), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(stamp, 42); EXPECT_STREQ(local, "unchanged"); EXPECT_STREQ(out, "unchanged");
    }
}
TEST_F(ConfigLoad, TimezoneLocaltimeFailureDoesNotPublishTimezoneOrTimestamp) {
    ASSERT_EQ(config_manager_init(), ESP_OK); force_clock = true; localtime_failure = true;
    char out[64] = "unchanged", local[64] = "unchanged"; time_t stamp = 42;
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), out, sizeof(out)), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(stamp, 42); EXPECT_STREQ(local, "unchanged"); EXPECT_STREQ(out, "unchanged");
}
TEST_F(ConfigLoad, TimezoneFailedReloadRetainsLastConfirmedCacheAndOldApplication) {
    ASSERT_EQ(config_manager_init(), ESP_OK); ASSERT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    text(NVS_TIMEZONE_KEY, "UTC1"); fail_key = NVS_AUTO_ROTATE_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL); char out[64]; bool applied = false;
    EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_OK);
    EXPECT_STREQ(out, "CST-8"); EXPECT_TRUE(applied); EXPECT_STREQ(getenv("TZ"), "CST-8");
}
TEST_F(ConfigLoad, TimezoneMalformedStoredShapeRefusesStartupWithoutWrites) {
    for (auto value : {"", "UTC0\n", "UTC\x7f", "UTC\xc2\xa0"}) {
        text(NVS_TIMEZONE_KEY, value); EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
    }
    EXPECT_EQ(env_calls, 0);
}
TEST_F(ConfigLoad, TimezoneCopyNullAndSmallOutputsRefuseWithoutPublication) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char out[64] = "unchanged", local[64] = "unchanged"; bool applied = false; time_t stamp = 42;
    EXPECT_EQ(config_manager_copy_timezone(nullptr, 64, &applied), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_timezone(out, 64, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_local_time(nullptr, local, 64, out, 64), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_local_time(&stamp, nullptr, 64, out, 64), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, 64, nullptr, 64), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, 64, out, 4), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, 0, out, 64), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, 19, out, 64), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(out, "unchanged"); EXPECT_EQ(stamp, 42); EXPECT_FALSE(applied);
}
TEST_F(ConfigLoad, TimezoneReloadAbsentRuleUsesDefaultWithoutPersistingIt) {
    text(NVS_TIMEZONE_KEY, "CST-8"); ASSERT_EQ(config_manager_init(), ESP_OK);
    values.erase(NVS_TIMEZONE_KEY); ASSERT_EQ(config_manager_init(), ESP_OK);
    char out[64]; bool applied = false;
    EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_OK); EXPECT_STREQ(out, "UTC0"); EXPECT_TRUE(applied);
    EXPECT_STREQ(getenv("TZ"), "UTC0"); read_only();
}
TEST_F(ConfigLoad, TimezoneLocalSnapshotMatchesZoneAndStaysIndependentAfterChange) {
    ASSERT_EQ(config_manager_init(), ESP_OK); ASSERT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    force_clock = true; synthetic_time = 1800000000;
    char out[64], local[64], expected[64]; tm candidate{}; time_t stamp = 42;
    ASSERT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), out, sizeof(out)), ESP_OK);
    localtime_r(&synthetic_time, &candidate); strftime(expected, sizeof(expected), "%Y-%m-%d %H:%M:%S", &candidate);
    EXPECT_STREQ(local, expected);
    EXPECT_EQ(stamp, synthetic_time); EXPECT_STREQ(out, "CST-8");
    ASSERT_EQ(config_manager_set_timezone("UTC0"), ESP_OK);
    EXPECT_STREQ(out, "CST-8"); EXPECT_STREQ(local, expected);
}
TEST_F(ConfigLoad, TimezoneStrftimeFailureDoesNotPublishAnyTimeOutput) {
    ASSERT_EQ(config_manager_init(), ESP_OK); strftime_failure = true;
    char out[64] = "unchanged", local[64] = "unchanged"; time_t stamp = 42;
    EXPECT_EQ(config_manager_copy_local_time(&stamp, local, sizeof(local), out, sizeof(out)), ESP_ERR_INVALID_STATE);
    EXPECT_STREQ(local, "unchanged"); EXPECT_STREQ(out, "unchanged"); EXPECT_EQ(stamp, 42);
}
TEST_F(ConfigLoad, TimezoneReloadApplicationFailureRetainsOldCacheButCannotClaimApplied) {
    ASSERT_EQ(config_manager_init(), ESP_OK); ASSERT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    text(NVS_TIMEZONE_KEY, "UTC1"); env_error = 1;
    EXPECT_EQ(config_manager_init(), ESP_ERR_NOT_FINISHED); char out[64]; bool applied = true;
    EXPECT_EQ(config_manager_copy_timezone(out, sizeof(out), &applied), ESP_OK); EXPECT_STREQ(out, "CST-8"); EXPECT_FALSE(applied);
}
TEST_F(ConfigLoad, NtpCopyBeforeLoadUsesDefaultWithoutIo) {
    char snapshot[NTP_SERVER_MAX_LEN];
    ASSERT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, DEFAULT_NTP_SERVER); EXPECT_EQ(opens, 0); read_only();
}
TEST_F(ConfigLoad, NtpSuccessfulSavePublishesOnlyAfterSingleCommitAndClose) {
    text(NVS_NTP_SERVER_KEY, "previous.example"); ASSERT_EQ(config_manager_init(), ESP_OK);
    int old_closes = closes;
    ASSERT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_OK);
    char snapshot[NTP_SERVER_MAX_LEN];
    ASSERT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, "next.example"); EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, "next.example");
    EXPECT_EQ(write_opens, 1); EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1); EXPECT_EQ(closes, old_closes + 1);
}
TEST_F(ConfigLoad, NtpOpenFailureLeavesRamAndDoesNotSetCommitOrClose) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int old_closes = closes;
    open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_FAIL);
    EXPECT_STREQ(config_manager_get_ntp_server(), DEFAULT_NTP_SERVER);
    EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0); EXPECT_EQ(closes, old_closes);
    open_error = ESP_OK; EXPECT_EQ(config_manager_set_ntp_server_checked("explicit.example"), ESP_OK);
}
TEST_F(ConfigLoad, NtpSetFailureClosesWithoutCommitOrRamPublication) {
    ASSERT_EQ(config_manager_init(), ESP_OK); save_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_ERR_NO_MEM);
    EXPECT_STREQ(config_manager_get_ntp_server(), DEFAULT_NTP_SERVER);
    EXPECT_EQ(commits, 0); EXPECT_TRUE(handles.empty());
}
TEST_F(ConfigLoad, NtpCommitFailureDoesNotClaimStorageRollbackOrPublishRam) {
    ASSERT_EQ(config_manager_init(), ESP_OK); commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_FAIL);
    EXPECT_STREQ(config_manager_get_ntp_server(), DEFAULT_NTP_SERVER);
    EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, "next.example"); // Synthetic set already took effect.
    EXPECT_EQ(commits, 1); EXPECT_TRUE(handles.empty());
    commit_error = ESP_OK; ASSERT_EQ(config_manager_set_ntp_server_checked("explicit.example"), ESP_OK);
    EXPECT_STREQ(config_manager_get_ntp_server(), "explicit.example");
}
TEST_F(ConfigLoad, NtpSnapshotsInputBeforeStorageCanMutateRequestBuffer) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char request[] = "next.example";
    ntp_io_hook = [&] { memset(request, 'x', sizeof(request) - 1); };
    ASSERT_EQ(config_manager_set_ntp_server_checked(request), ESP_OK);
    EXPECT_STREQ(config_manager_get_ntp_server(), "next.example");
    EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, "next.example");
}
TEST_F(ConfigLoad, NtpBusyRejectsCopySetAndInitWithoutOutputOrExtraIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); char snapshot[128] = "unchanged";
    ntp_io_hook = [&] {
        EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_set_ntp_server_checked("nested.example"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
        config_manager_set_ntp_server("void-nested.example");
        EXPECT_STREQ(snapshot, "unchanged");
    };
    ASSERT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_OK);
    EXPECT_EQ(opens, 2); EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1);
    EXPECT_STREQ(config_manager_get_ntp_server(), "next.example");
}
TEST_F(ConfigLoad, NtpLoadingRejectsConcurrentCheckedSaveAndCopy) {
    char snapshot[128] = "unchanged";
    ntp_io_hook = [&] {
        EXPECT_EQ(config_manager_set_ntp_server_checked("nested.example"), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_ERR_INVALID_STATE);
    };
    ASSERT_EQ(config_manager_init(), ESP_OK); EXPECT_STREQ(snapshot, "unchanged"); read_only();
}
TEST_F(ConfigLoad, NtpCheckedReadersAndWritersRejectActualConcurrentStorageOperation) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    std::atomic<int> stage{0}; esp_err_t result = ESP_FAIL;
    ntp_io_hook = [&] {
        stage.store(1);
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (stage.load() != 2 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
    };
    std::thread writer([&] { result = config_manager_set_ntp_server_checked("next.example"); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (stage.load() != 1 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    EXPECT_EQ(stage.load(), 1);
    char snapshot[128] = "unchanged";
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(config_manager_set_ntp_server_checked("other.example"), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_STATE);
    EXPECT_STREQ(snapshot, "unchanged");
    stage.store(2); writer.join();
    EXPECT_EQ(result, ESP_OK);
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, "next.example"); EXPECT_EQ(writes, 1); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, NtpCopyIsCallerOwnedAndExactCapacityChecked) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    char snapshot[NTP_SERVER_MAX_LEN]; ASSERT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    ASSERT_EQ(config_manager_set_ntp_server_checked("next.example"), ESP_OK);
    EXPECT_STREQ(snapshot, DEFAULT_NTP_SERVER);
    strcpy(snapshot, "unchanged");
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, strlen("next.example")), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(snapshot, "unchanged");
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, strlen("next.example") + 1), ESP_OK);
    EXPECT_STREQ(snapshot, "next.example");
    EXPECT_EQ(config_manager_copy_ntp_server(nullptr, 128), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, 0), ESP_ERR_INVALID_SIZE);
    EXPECT_STREQ(snapshot, "next.example");
}
TEST_F(ConfigLoad, NtpEmptyValueRetainsExplicitDefaultFallback) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ASSERT_EQ(config_manager_set_ntp_server_checked(""), ESP_OK);
    EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, "");
    char snapshot[128]; ASSERT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, DEFAULT_NTP_SERVER);
    ASSERT_EQ(config_manager_init(), ESP_OK); EXPECT_STREQ(config_manager_get_ntp_server(), DEFAULT_NTP_SERVER);
}
TEST_F(ConfigLoad, NtpMaximumLengthAcceptedWithoutTruncationAndLongOrNullRejected) {
    ASSERT_EQ(config_manager_init(), ESP_OK); std::string longest(127, 'a');
    EXPECT_EQ(config_manager_set_ntp_server_checked(longest.c_str()), ESP_OK);
    int before = opens;
    EXPECT_EQ(config_manager_set_ntp_server_checked(std::string(128, 'b').c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(config_manager_set_ntp_server_checked(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, before); EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, longest);
    char snapshot[128]; EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_EQ(std::string(snapshot), longest);
}
TEST_F(ConfigLoad, NtpWhitespaceControlAndNonAsciiRejectedBeforeIo) {
    ASSERT_EQ(config_manager_init(), ESP_OK); int before = opens;
    for (const char *value : {"bad host", "host\n", "\thost", "host\x7f", "host\xc2\xa0"})
        EXPECT_EQ(config_manager_set_ntp_server_checked(value), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, before); EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ConfigLoad, NtpFailedReloadRetainsLastConfirmedSnapshot) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ASSERT_EQ(config_manager_set_ntp_server_checked("previous.example"), ESP_OK);
    text(NVS_NTP_SERVER_KEY, "unpublished.example"); fail_key = NVS_AUTO_ROTATE_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL);
    char snapshot[128]; EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, "previous.example");
    fail_key.clear(); ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_copy_ntp_server(snapshot, sizeof(snapshot)), ESP_OK);
    EXPECT_STREQ(snapshot, "unpublished.example");
}
TEST_F(ConfigLoad, NtpMalformedLoadedTextRefusesStartupWithoutWrite) {
    for (const char *value : {"bad host", "host\n", "host\x7f", "host\xc2\xa0"}) {
        text(NVS_NTP_SERVER_KEY, value);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
    }
}
TEST_F(ConfigLoad, NtpVoidCompatibilitySaveAlsoDoesNotPublishFailedWrite) {
    ASSERT_EQ(config_manager_init(), ESP_OK); commit_error = ESP_FAIL;
    config_manager_set_ntp_server("next.example");
    EXPECT_STREQ(config_manager_get_ntp_server(), DEFAULT_NTP_SERVER); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, FreshNamespaceDefaultsAreCompleteAndDoNotWrite) {
    namespace_present = false;
    ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_STREQ(config_manager_get_device_name(), DEFAULT_DEVICE_NAME);
    EXPECT_STREQ(config_manager_get_timezone(), DEFAULT_TIMEZONE);
    EXPECT_STREQ(config_manager_get_image_url(), DEFAULT_IMAGE_URL);
    EXPECT_EQ(config_manager_get_display_rotation_deg(), 180);
    EXPECT_EQ(config_manager_get_rotation_mode(), ROTATION_MODE_STORAGE);
    EXPECT_FALSE(config_manager_get_auto_rotate()); EXPECT_STREQ(config_manager_get_http_password(), "");
    EXPECT_EQ(opens, 1); EXPECT_EQ(closes, 0); read_only();
}
TEST_F(ConfigLoad, BeforeMountDefaultIsRequiredSdNotLegacyVolatileUrlFallback) {
    persistent = false;
    ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_get_rotation_mode(), ROTATION_MODE_STORAGE);
    read_only();
}
TEST_F(ConfigLoad, CredentialPairAbsentOrBothEmptyIsNotFoundWithoutChangingCaller) {
    char ssid[33] = "previous", password[65] = "previous";
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_NVS_NOT_FOUND);
    namespace_present = false;
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_NVS_NOT_FOUND);
    namespace_present = true; text(NVS_WIFI_SSID_KEY, ""); text(NVS_WIFI_PASS_KEY, "");
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_NVS_NOT_FOUND);
    EXPECT_STREQ(ssid, "previous"); EXPECT_STREQ(password, "previous"); read_only();
}
TEST_F(ConfigLoad, PartialCredentialPairIsNotAnUnconfiguredDevice) {
    char ssid[33] = "previous", password[65] = "previous";
    text(NVS_WIFI_SSID_KEY, "network");
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_INVALID_RESPONSE);
    values.clear(); text(NVS_WIFI_PASS_KEY, "password");
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_INVALID_RESPONSE);
    EXPECT_STREQ(ssid, "previous"); EXPECT_STREQ(password, "previous"); read_only();
}
TEST_F(ConfigLoad, CredentialPairFullWireAndOpenPasswordAreCompleteReads) {
    char ssid[33], password[65];
    text(NVS_WIFI_SSID_KEY, std::string(32, 's')); text(NVS_WIFI_PASS_KEY, std::string(64, 'f'));
    ASSERT_EQ(wifi_manager_load_credentials(ssid, password), ESP_OK);
    EXPECT_EQ(strlen(ssid), 32u); EXPECT_EQ(strlen(password), 64u);
    text(NVS_WIFI_PASS_KEY, ""); ASSERT_EQ(wifi_manager_load_credentials(ssid, password), ESP_OK);
    EXPECT_STREQ(password, ""); read_only();
}
TEST_F(ConfigLoad, CredentialReadErrorsTypesLengthsAndMalformedStringsNeverPublishPartialOutput) {
    char ssid[33] = "previous", password[65] = "previous";
    text(NVS_WIFI_SSID_KEY, "network"); text(NVS_WIFI_PASS_KEY, "password");
    for (const char *key : {NVS_WIFI_SSID_KEY, NVS_WIFI_PASS_KEY}) {
        fail_key = key; fail_error = ESP_FAIL;
        EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_FAIL);
        EXPECT_STREQ(ssid, "previous"); EXPECT_STREQ(password, "previous");
    }
    fail_key.clear(); number(NVS_WIFI_PASS_KEY, U8, 1);
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_NVS_TYPE_MISMATCH);
    text(NVS_WIFI_PASS_KEY, std::string(65, 'p'));
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_NVS_INVALID_LENGTH);
    text(NVS_WIFI_PASS_KEY, std::string("password\0tail", 13));
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_INVALID_RESPONSE);
    EXPECT_STREQ(ssid, "previous"); EXPECT_STREQ(password, "previous"); read_only();
}
TEST_F(ConfigLoad, InvalidCredentialValuesAndNullOutputsFailBeforeUse) {
    char ssid[33] = "previous", password[65] = "previous";
    EXPECT_EQ(wifi_manager_load_credentials(nullptr, password), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, 0);
    text(NVS_WIFI_SSID_KEY, "network"); text(NVS_WIFI_PASS_KEY, "short");
    EXPECT_EQ(wifi_manager_load_credentials(ssid, password), ESP_ERR_INVALID_RESPONSE);
    EXPECT_STREQ(ssid, "previous"); EXPECT_STREQ(password, "previous"); read_only();
}
TEST_F(ConfigLoad, CredentialSaveChecksInputOpenSetAndCommitWithoutClaimingRollback) {
    EXPECT_EQ(wifi_manager_save_credentials("network", "short"), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, 0);
    open_error = ESP_FAIL; EXPECT_EQ(wifi_manager_save_credentials("network", "password"), ESP_FAIL);
    open_error = 0; save_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_save_credentials("network", "password"), ESP_FAIL);
    save_error = 0; commit_error = ESP_FAIL;
    EXPECT_EQ(wifi_manager_save_credentials("network", "password"), ESP_FAIL);
    EXPECT_TRUE(handles.empty());
    commit_error = 0; ASSERT_EQ(wifi_manager_save_credentials("network", "password"), ESP_OK);
    EXPECT_EQ(values.at(NVS_WIFI_SSID_KEY).text, "network");
}
TEST_F(ConfigLoad, CheckedNameSavePublishesLiveValueOnlyAfterSuccessfulCommit) {
    text(NVS_DEVICE_NAME_KEY, "previous"); ASSERT_EQ(config_manager_init(), ESP_OK);
    ASSERT_EQ(config_manager_set_device_name_checked("new"), ESP_OK);
    EXPECT_STREQ(config_manager_get_device_name(), "new"); EXPECT_EQ(commits, 1);
    EXPECT_EQ(write_opens, 1); EXPECT_TRUE(handles.empty());
}
TEST_F(ConfigLoad, CheckedProvisioningSavesEightFieldsOnceBeforePublishingRam) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    ASSERT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_OK);
    EXPECT_EQ(write_opens, 1); EXPECT_EQ(writes, 8); EXPECT_EQ(commits, 1); EXPECT_EQ(erases, 0);
    EXPECT_STREQ(config_manager_get_device_name(), "new");
    EXPECT_STREQ(config_manager_get_wifi_ssid(), "network");
    EXPECT_STREQ(config_manager_get_wifi_password(), "password");
    EXPECT_EQ(config_manager_get_ip_mode(), IP_MODE_STATIC);
    EXPECT_STREQ(config_manager_get_static_ip(), network.ip);
    EXPECT_STREQ(config_manager_get_static_netmask(), network.netmask);
    EXPECT_STREQ(config_manager_get_static_gateway(), network.gateway);
    EXPECT_STREQ(config_manager_get_dns_server(), network.dns);
    char ssid[33], password[65]; ASSERT_EQ(wifi_manager_load_credentials(ssid, password), ESP_OK);
    EXPECT_STREQ(ssid, "network"); EXPECT_STREQ(password, "password");
}
TEST_F(ConfigLoad, CheckedProvisioningOpenErrorDoesNotChangeConfigOrWrite) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    wifi_network_t network{};
    open_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_ERR_NO_MEM);
    EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0);
    EXPECT_STREQ(config_manager_get_device_name(), DEFAULT_DEVICE_NAME);
    EXPECT_STREQ(config_manager_get_wifi_ssid(), DEFAULT_WIFI_SSID);
}
TEST_F(ConfigLoad, EachProvisioningSetErrorStopsBeforeCommitAndKeepsWholeRamGroup) {
    ASSERT_EQ(config_manager_init(), ESP_OK);
    auto previous_name = std::string(config_manager_get_device_name());
    auto previous_ssid = std::string(config_manager_get_wifi_ssid());
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    const char *keys[] = {NVS_WIFI_SSID_KEY, NVS_WIFI_PASS_KEY, NVS_DEVICE_NAME_KEY,
                         NVS_STATIC_IP_KEY, NVS_STATIC_NETMASK_KEY, NVS_STATIC_GATEWAY_KEY,
                         NVS_DNS_SERVER_KEY, NVS_IP_MODE_KEY};
    for (unsigned i = 0; i < 8; ++i) {
        fail_write_key = keys[i]; int before = writes;
        EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_ERR_NO_MEM);
        EXPECT_EQ(writes - before, (int)i + 1); EXPECT_EQ(commits, 0); EXPECT_EQ(erases, 0);
        EXPECT_STREQ(config_manager_get_device_name(), previous_name.c_str());
        EXPECT_STREQ(config_manager_get_wifi_ssid(), previous_ssid.c_str());
        EXPECT_EQ(config_manager_get_ip_mode(), IP_MODE_DHCP);
        EXPECT_STREQ(config_manager_get_static_ip(), ""); EXPECT_STREQ(config_manager_get_dns_server(), "");
        EXPECT_TRUE(handles.empty());
    }
}
TEST_F(ConfigLoad, ProvisioningCommitErrorReportsPersistentPartialWithoutRamPublishOrErase) {
    ASSERT_EQ(config_manager_init(), ESP_OK); wifi_network_t network{};
    commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_FAIL);
    EXPECT_EQ(commits, 1); EXPECT_EQ(writes, 8); EXPECT_EQ(erases, 0);
    EXPECT_STREQ(config_manager_get_device_name(), DEFAULT_DEVICE_NAME);
    EXPECT_EQ(values.at(NVS_DEVICE_NAME_KEY).text, "new"); // Stub persists at set: no rollback claim.
    commit_error = ESP_OK;
    ASSERT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_OK);
    EXPECT_STREQ(config_manager_get_device_name(), "new");
}
TEST_F(ConfigLoad, InvalidProvisioningInputsHaveNoNvsEffects) {
    wifi_network_t network{};
    EXPECT_EQ(config_manager_save_provisioning_checked(nullptr, "password", "new", &network), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "short", "new", &network), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "", &network), ESP_ERR_INVALID_ARG);
    auto long_name = std::string(64, 'n');
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", long_name.c_str(), &network), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", nullptr), ESP_ERR_INVALID_ARG);
    network.static_address = true;
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(opens, 0); EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ConfigLoad, CheckedProvisioningKeepsFullWireFieldsAndDhcpRetainedStaticValues) {
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, false, "192.168.1.42", "255.255.255.0", "192.168.1.1", ""));
    auto ssid = std::string(32, 's'), password = std::string(64, 'f'), name = std::string(63, 'n');
    ASSERT_EQ(config_manager_save_provisioning_checked(ssid.c_str(), password.c_str(), name.c_str(), &network), ESP_OK);
    EXPECT_STREQ(config_manager_get_wifi_ssid(), ssid.c_str());
    EXPECT_STREQ(config_manager_get_wifi_password(), password.c_str());
    EXPECT_STREQ(config_manager_get_device_name(), name.c_str());
    EXPECT_EQ(config_manager_get_ip_mode(), IP_MODE_DHCP);
    EXPECT_STREQ(config_manager_get_static_ip(), "192.168.1.42");
}
TEST_F(ConfigLoad, ProvisioningInputSnapshotCannotChangeDuringNvsOrAliasPublishedStrings) {
    wifi_network_t network{};
    ASSERT_TRUE(wifi_network_copy(&network, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    char ssid[33] = "network", password[65] = "password", name[64] = "new";
    mutate_ssid = ssid; mutate_password = password; mutate_name = name; mutate_network = &network;
    ASSERT_EQ(config_manager_save_provisioning_checked(ssid, password, name, &network), ESP_OK);
    EXPECT_STREQ(ssid, ""); EXPECT_STREQ(config_manager_get_wifi_ssid(), "network");
    EXPECT_STREQ(config_manager_get_wifi_password(), "password");
    EXPECT_STREQ(config_manager_get_device_name(), "new");
    EXPECT_STREQ(config_manager_get_dns_server(), "8.8.8.8"); EXPECT_EQ(config_manager_get_ip_mode(), IP_MODE_STATIC);
    mutate_network = nullptr;
    ASSERT_TRUE(wifi_network_copy(&network, false, "", "", "", ""));
    EXPECT_EQ(config_manager_save_provisioning_checked(config_manager_get_wifi_ssid(),
              config_manager_get_wifi_password(), config_manager_get_device_name(), &network), ESP_OK);
    EXPECT_STREQ(config_manager_get_wifi_ssid(), "network");
}
TEST_F(ConfigLoad, CheckedNameSaveErrorsKeepLiveValueAndReportPossiblePersistentPartialResult) {
    text(NVS_DEVICE_NAME_KEY, "previous"); ASSERT_EQ(config_manager_init(), ESP_OK);
    open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_device_name_checked("new"), ESP_FAIL);
    open_error = 0; save_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_device_name_checked("new"), ESP_FAIL);
    save_error = 0; commit_error = ESP_FAIL;
    EXPECT_EQ(config_manager_set_device_name_checked("new"), ESP_FAIL);
    EXPECT_STREQ(config_manager_get_device_name(), "previous"); EXPECT_TRUE(handles.empty());
    // The commit failure does not imply rollback of earlier set operations.
}
TEST_F(ConfigLoad, InvalidNameDoesNotOpenNvsOrTruncate) {
    EXPECT_EQ(config_manager_set_device_name_checked(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_set_device_name_checked(""), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_manager_set_device_name_checked(std::string(64, 'n').c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(opens, 0);
}
TEST_F(ConfigLoad, ExistingValidSettingsAndLegacyKeysAreReadWithoutMutation) {
    legacy_keys(); text(NVS_DEVICE_NAME_KEY, "synthetic frame"); text(NVS_HTTP_PASSWORD_KEY, "test-only");
    text(NVS_TIMEZONE_KEY, "CST-8"); text(NVS_ROTATE_CRON_KEY, "0 */12 *\n30 8 1-5");
    number(NVS_DISPLAY_ROTATION_DEG_KEY, I32, 0); number(NVS_DISPLAY_ORIENTATION_KEY, U8, 1);
    number(NVS_AUTO_ROTATE_KEY, U8, 1);
    ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_EQ(config_manager_get_cron_rule_count(), 2); EXPECT_TRUE(config_manager_get_auto_rotate());
    EXPECT_EQ(config_manager_get_display_rotation_deg(), 0); // Do not silently rewrite an intentional override.
    EXPECT_EQ(config_manager_get_display_orientation(), DISPLAY_ORIENTATION_PORTRAIT);
    EXPECT_STREQ(config_manager_get_http_password(), "test-only");
    EXPECT_TRUE(values.count(NVS_SLEEP_SCHEDULE_START_KEY)); EXPECT_EQ(opens, 1); EXPECT_EQ(closes, 1);
    read_only();
}
TEST_F(ConfigLoad, LegacyIntervalIsOnlyPreviewedInRam) {
    legacy_keys(); number(NVS_ROTATE_INTERVAL_KEY, I32, 7200);
    ASSERT_EQ(config_manager_init(), ESP_OK);
    ASSERT_EQ(config_manager_get_cron_rule_count(), 1);
    EXPECT_STREQ(config_manager_get_cron_rule(0), "0 */2 *");
    EXPECT_FALSE(values.count(NVS_ROTATE_CRON_KEY)); EXPECT_TRUE(values.count(NVS_ROTATE_INTERVAL_KEY));
    EXPECT_TRUE(values.count(NVS_SLEEP_SCHEDULE_ENABLED_KEY)); read_only();
}
TEST_F(ConfigLoad, NamespaceReadFailureNeverFallsBackToUnauthenticatedStartup) {
    open_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL); EXPECT_EQ(reads, 0); read_only();
}
TEST_F(ConfigLoad, HttpPasswordTypeSizeAndIoErrorsRefuseLoad) {
    for (esp_err_t error : {ESP_FAIL, ESP_ERR_NVS_TYPE_MISMATCH, ESP_ERR_NVS_INVALID_LENGTH}) {
        fail_key = NVS_HTTP_PASSWORD_KEY; fail_error = error;
        EXPECT_EQ(config_manager_init(), error); read_only();
    }
    fail_key.clear(); text(NVS_HTTP_PASSWORD_KEY, std::string(HTTP_PASSWORD_MAX_LEN, 'a'));
    EXPECT_EQ(config_manager_init(), ESP_ERR_NVS_INVALID_LENGTH); read_only();
}
TEST_F(ConfigLoad, EveryLoadedStringPropagatesReadFailureAndLeavesLegacyKeys) {
    legacy_keys();
    for (const char *key : {NVS_DEVICE_NAME_KEY, NVS_TIMEZONE_KEY, NVS_NTP_SERVER_KEY,
            NVS_STATIC_IP_KEY, NVS_STATIC_NETMASK_KEY, NVS_STATIC_GATEWAY_KEY, NVS_DNS_SERVER_KEY,
            NVS_WIFI_SSID_KEY, NVS_WIFI_PASS_KEY, NVS_ROTATE_CRON_KEY, NVS_IMAGE_URL_KEY,
            NVS_ACCESS_TOKEN_KEY, NVS_HTTP_HEADER_KEY_KEY, NVS_HTTP_HEADER_VALUE_KEY,
            NVS_IMAGE_ETAG_KEY, NVS_HA_URL_KEY, NVS_OPENAI_API_KEY_KEY, NVS_GOOGLE_API_KEY_KEY}) {
        fail_key = key; fail_error = ESP_FAIL;
        EXPECT_EQ(config_manager_init(), ESP_FAIL) << key;
        EXPECT_TRUE(values.count(NVS_SLEEP_SCHEDULE_START_KEY)); read_only();
    }
}
TEST_F(ConfigLoad, OutOfRangeEnumsBooleansAndScalarsRefuseLoadWithoutRepair) {
    for (const char *key : {NVS_IP_MODE_KEY, NVS_DISPLAY_ORIENTATION_KEY, NVS_AUTO_ROTATE_KEY,
            NVS_ROTATION_MODE_KEY, NVS_SD_ROTATION_MODE_KEY, NVS_SAVE_DOWNLOADED_KEY,
            NVS_DEEP_SLEEP_KEY, NVS_DEBUG_LOG_KEY}) {
        values.clear(); number(key, U8, 255);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE) << key; read_only();
    }
    for (const auto &entry : std::vector<std::pair<const char *, int>>{
            {NVS_DISPLAY_ROTATION_DEG_KEY, 90}, {NVS_DISPLAY_ROTATION_DEG_KEY, 270},
            {NVS_DISPLAY_ROTATION_DEG_KEY, -1}, {NVS_LAST_INDEX_KEY, -2}, {NVS_ROTATE_INTERVAL_KEY, 0}}) {
        values.clear(); number(entry.first, I32, entry.second);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
    }
    values.clear(); number("cfg_updated", I64, -1);
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
}
TEST_F(ConfigLoad, WrongScalarTypesAndReadIoErrorsAreNotDefaults) {
    for (const char *key : {NVS_AUTO_ROTATE_KEY, NVS_DISPLAY_ROTATION_DEG_KEY,
                           NVS_LAST_INDEX_KEY, "cfg_updated"}) {
        values.clear(); text(key, "bad-type");
        EXPECT_EQ(config_manager_init(), ESP_ERR_NVS_TYPE_MISMATCH); read_only();
        values.clear(); fail_key = key; fail_error = ESP_FAIL;
        EXPECT_EQ(config_manager_init(), ESP_FAIL); read_only(); fail_key.clear();
    }
}
TEST_F(ConfigLoad, UnterminatedOrEmbeddedNulStringCannotBecomeAValidCredential) {
    text(NVS_HTTP_PASSWORD_KEY, "test-only"); malformed_string = true;
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
    malformed_string = false; text(NVS_HTTP_PASSWORD_KEY, std::string("a\0b", 3));
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
}
TEST_F(ConfigLoad, PinnedCaBoundedLoadAndBothReadFailuresRefuseUnpinnedStartup) {
    values[NVS_CA_CERT_KEY] = {Blob, std::string(4096, 'x')};
    ASSERT_EQ(config_manager_init(), ESP_OK);
    size_t length; ASSERT_NE(config_manager_get_ca_cert_der(&length), nullptr); EXPECT_EQ(length, 4096u);
    values[NVS_CA_CERT_KEY].text.push_back('x');
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_SIZE); read_only();
    values[NVS_CA_CERT_KEY].text.clear();
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_SIZE); read_only();
    values[NVS_CA_CERT_KEY].text = "test cert";
    max_allocation = 1;
    EXPECT_EQ(config_manager_init(), ESP_ERR_NO_MEM); read_only();
    max_allocation = SIZE_MAX; blob_calls = 0; fail_blob_call = 2;
    EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE); read_only();
    fail_blob_call = 0; fail_key = NVS_CA_CERT_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_FAIL); read_only();
}
TEST_F(ConfigLoad, SubsequentSyntheticBootCannotReuseAbsentPasswordOrPinnedCa) {
    text(NVS_HTTP_PASSWORD_KEY, "test-only"); values[NVS_CA_CERT_KEY] = {Blob, "test cert"};
    ASSERT_EQ(config_manager_init(), ESP_OK);
    values.clear(); ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_STREQ(config_manager_get_http_password(), "");
    size_t length = 99; EXPECT_EQ(config_manager_get_ca_cert_der(&length), nullptr); EXPECT_EQ(length, 0u);
    read_only();
}
TEST_F(ConfigLoad, InvalidOrSilentlyDroppedCronRulesRefuseLoadAndKeepStoredBytes) {
    for (const std::string &rules : {std::string("bad cron"), std::string(64, '*'),
                std::string("0 0 *\n0 0 *\n0 0 *\n0 0 *\n0 0 *\n0 0 *\n0 0 *\n0 0 *")}) {
        text(NVS_ROTATE_CRON_KEY, rules);
        EXPECT_EQ(config_manager_init(), ESP_ERR_INVALID_RESPONSE);
        EXPECT_EQ(values.at(NVS_ROTATE_CRON_KEY).text, rules); read_only();
    }
}
#else
TEST_F(ConfigLoad, LegacyRevisionVoidSetRetainsRamFirstOnCommitFailure) {
    commit_error = ESP_FAIL; config_manager_set_config_last_updated(1900000000);
    EXPECT_EQ(config_manager_get_config_last_updated(), 1900000000); EXPECT_EQ(commits, 1);
}
TEST_F(ConfigLoad, LegacyTimezoneSaveRetainsRamFirstAndCallerAppliedEnvironment) {
    ASSERT_EQ(config_manager_init(), ESP_OK); commit_error = ESP_FAIL; int before = env_calls;
    EXPECT_EQ(config_manager_set_timezone("CST-8"), ESP_OK);
    EXPECT_STREQ(config_manager_get_timezone(), "CST-8"); EXPECT_STREQ(getenv("TZ"), DEFAULT_TIMEZONE);
    EXPECT_EQ(env_calls, before); EXPECT_EQ(values.at(NVS_TIMEZONE_KEY).text, "CST-8");
}
TEST_F(ConfigLoad, LegacyNtpVoidSaveRetainsUpstreamTruncationAndRamFirstBehavior) {
    ASSERT_EQ(config_manager_init(), ESP_OK); commit_error = ESP_FAIL;
    std::string overlong(NTP_SERVER_MAX_LEN + 10, 'a');
    config_manager_set_ntp_server(overlong.c_str());
    EXPECT_EQ(strlen(config_manager_get_ntp_server()), NTP_SERVER_MAX_LEN - 1u);
    EXPECT_EQ(values.at(NVS_NTP_SERVER_KEY).text, std::string(NTP_SERVER_MAX_LEN - 1, 'a'));
    EXPECT_TRUE(handles.empty());
}
TEST_F(ConfigLoad, LegacyBoardDoesNotRunNewGroupedProvisioningSave) {
    wifi_network_t network{};
    EXPECT_EQ(config_manager_save_provisioning_checked("network", "password", "new", &network), ESP_ERR_NOT_SUPPORTED);
    EXPECT_EQ(opens, 0); EXPECT_EQ(writes, 0); EXPECT_EQ(commits, 0);
}
TEST_F(ConfigLoad, LegacyBoardStillPersistsIntervalAndDeletesQuietHourKeys) {
    legacy_keys(); number(NVS_ROTATE_INTERVAL_KEY, I32, 7200);
    ASSERT_EQ(config_manager_init(), ESP_OK);
    EXPECT_TRUE(values.count(NVS_ROTATE_CRON_KEY)); EXPECT_FALSE(values.count(NVS_SLEEP_SCHEDULE_START_KEY));
    EXPECT_GE(writes, 1); EXPECT_GE(commits, 1); EXPECT_GE(erases, 3);
}
TEST_F(ConfigLoad, LegacyBoardRetainsItsOriginalPasswordReadErrorPolicy) {
    fail_key = NVS_HTTP_PASSWORD_KEY; fail_error = ESP_FAIL;
    EXPECT_EQ(config_manager_init(), ESP_OK); EXPECT_STREQ(config_manager_get_http_password(), "");
}
#endif
