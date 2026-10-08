#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>

extern "C" {
#include "read_pico_sd.h"
#include "storage.h"
}

namespace
{
int mount_calls, unmount_calls, fallback_calls, device_name_calls;
esp_err_t mount_result, unmount_result, status_result;
read_pico_sd_status_t status;
bool close_failed, fail_close, lose_on_close;
int close_calls;
}  // namespace

extern "C" int wifi_storage_test_fclose(FILE *file)
{
    ++close_calls;
    int result = fclose(file);
    if (lose_on_close) status.media_valid = false;
    return fail_close ? EOF : result;
}
extern "C" void read_pico_sd_note_file_close_failure(void)
{
    close_failed = true; status.exit_failed = true; status.media_valid = false;
}

extern "C" esp_err_t read_pico_sd_mount(void)
{
    ++mount_calls;
    return mount_result;
}
extern "C" esp_err_t read_pico_sd_unmount(void)
{
    ++unmount_calls;
    return unmount_result;
}
extern "C" esp_err_t read_pico_sd_get_status(read_pico_sd_status_t *out)
{
    *out = status;
    return status_result;
}
extern "C" esp_err_t memfs_mount(const char *, size_t)
{
    ++fallback_calls;
    return ESP_OK;
}
extern "C" void config_manager_set_device_name(const char *)
{
    ++device_name_calls;
}
extern "C" const char *esp_err_to_name(esp_err_t)
{
    return "host-error";
}

class ReadPicoStorage : public testing::Test
{
    std::filesystem::path previous_dir, scratch_dir;
   protected:
    void SetUp() override
    {
        unmount_result = ESP_OK;
        ASSERT_EQ(storage_unmount_checked(), ESP_OK);
        mount_calls = unmount_calls = fallback_calls = device_name_calls = 0;
        mount_result = status_result = ESP_OK;
        status = {true, true, true, false};
        fail_close = lose_on_close = close_failed = false; close_calls = 0;
        previous_dir = std::filesystem::current_path();
        char path[] = "/tmp/read-pico-wifi-file-XXXXXX";
        ASSERT_NE(mkdtemp(path), nullptr); scratch_dir = path;
        std::filesystem::current_path(scratch_dir);
        std::filesystem::create_directories("pf_wifi_storage/config");
    }
    void TearDown() override
    {
        unmount_result = ESP_OK;
        EXPECT_EQ(storage_unmount_checked(), ESP_OK);
        std::filesystem::current_path(previous_dir);
        std::filesystem::remove_all(scratch_dir);
    }
    void write(const char *path, const std::string &bytes) {
        std::ofstream f(path, std::ios::binary); f.write(bytes.data(), bytes.size());
    }
};

TEST_F(ReadPicoStorage, ProductMountUsesSoleOwnerAndNeverFallback)
{
    ASSERT_EQ(storage_init(), ESP_OK);
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(storage_get_type(), STORAGE_TYPE_SDCARD);
    EXPECT_TRUE(storage_has_persistent_storage());
    EXPECT_EQ(fallback_calls, 0);
    EXPECT_EQ(storage_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 1);
}
TEST_F(ReadPicoStorage, MountFailuresReturnOriginalErrorWithoutVolatileSubstitution)
{
    for (esp_err_t err : {ESP_ERR_NOT_FOUND, ESP_FAIL, ESP_ERR_TIMEOUT, ESP_ERR_INVALID_STATE}) {
        mount_result = err;
        EXPECT_EQ(storage_init(), err);
        EXPECT_EQ(storage_get_type(), STORAGE_TYPE_NONE);
        EXPECT_FALSE(storage_has_persistent_storage());
    }
    EXPECT_EQ(mount_calls, 4);
    EXPECT_EQ(fallback_calls, 0);
}
TEST_F(ReadPicoStorage, EveryInvalidMediaStateFailsPersistentAvailability)
{
    ASSERT_EQ(storage_init(), ESP_OK);
    for (auto invalid : {read_pico_sd_status_t{false, true, false, false},
                         read_pico_sd_status_t{true, false, true, false},
                         read_pico_sd_status_t{true, true, false, false},
                         read_pico_sd_status_t{true, true, true, true}}) {
        status = invalid;
        EXPECT_EQ(storage_get_type(), STORAGE_TYPE_NONE);
        EXPECT_FALSE(storage_has_persistent_storage());
    }
    status = {true, true, true, false};
    status_result = ESP_ERR_TIMEOUT;
    EXPECT_FALSE(storage_has_persistent_storage());
    EXPECT_EQ(unmount_calls, 0);  // Never silently tear down another consumer's mount.
}
TEST_F(ReadPicoStorage, RemovedMediaStillTeardownThroughOwner)
{
    ASSERT_EQ(storage_init(), ESP_OK);
    status = {false, true, false, false};
    EXPECT_EQ(storage_get_type(), STORAGE_TYPE_NONE);
    EXPECT_EQ(storage_unmount_checked(), ESP_OK);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(storage_init(), ESP_OK);
    EXPECT_EQ(mount_calls, 2);
}
TEST_F(ReadPicoStorage, FailedUnmountRetainsOwnershipAndPreventsNewMount)
{
    ASSERT_EQ(storage_init(), ESP_OK);
    unmount_result = ESP_ERR_TIMEOUT;
    status.exit_failed = true;
    EXPECT_EQ(storage_unmount_checked(), ESP_ERR_TIMEOUT);
    EXPECT_FALSE(storage_has_persistent_storage());
    EXPECT_EQ(storage_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 1);
}
TEST_F(ReadPicoStorage, FormatDisabledEvenWithValidOwnedMedia)
{
    EXPECT_EQ(storage_format(), ESP_ERR_NOT_SUPPORTED);
    ASSERT_EQ(storage_init(), ESP_OK);
    EXPECT_EQ(storage_format(), ESP_ERR_NOT_SUPPORTED);
    EXPECT_EQ(unmount_calls, 0);
}
TEST_F(ReadPicoStorage, InvalidMediaNeverReadsWifiFileOrChangesDeviceName)
{
    char ssid[32] = "preserved", password[64] = "preserved";
    EXPECT_EQ(storage_read_wifi_credentials(ssid, password), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(storage_init(), ESP_OK);
    status.media_valid = false;
    EXPECT_EQ(storage_read_wifi_credentials(ssid, password), ESP_ERR_INVALID_STATE);
    EXPECT_STREQ(ssid, "preserved");
    EXPECT_STREQ(password, "preserved");
    EXPECT_EQ(device_name_calls, 0);
}
TEST_F(ReadPicoStorage, CheckedImportReturnsNameAfterCloseWithoutSavingOrDeletingSource) {
    ASSERT_EQ(storage_init(), ESP_OK);
    write("pf_wifi_storage/config/wifi.txt", "network\npassword\nFrame\n");
    wifi_text_t out{}; ASSERT_EQ(storage_read_wifi_config(&out), ESP_OK);
    EXPECT_STREQ(out.ssid, "network"); EXPECT_STREQ(out.device_name, "Frame");
    EXPECT_EQ(close_calls, 1); EXPECT_EQ(device_name_calls, 0);
    EXPECT_TRUE(std::filesystem::exists("pf_wifi_storage/config/wifi.txt"));
}
TEST_F(ReadPicoStorage, OnlyAbsentPreferredFileAllowsRootFallback) {
    ASSERT_EQ(storage_init(), ESP_OK);
    write("pf_wifi_storage/wifi.txt", "network\npassword");
    wifi_text_t out{}; ASSERT_EQ(storage_read_wifi_config(&out), ESP_OK);
    EXPECT_STREQ(out.ssid, "network");
    write("pf_wifi_storage/config/wifi.txt", "bad file");
    EXPECT_EQ(storage_read_wifi_config(&out), ESP_ERR_INVALID_ARG);
    EXPECT_STREQ(out.ssid, "network"); EXPECT_EQ(device_name_calls, 0);
}
TEST_F(ReadPicoStorage, PreferredPathDirectoryOrInvalidParentDoesNotHideBehindRootFile) {
    ASSERT_EQ(storage_init(), ESP_OK); write("pf_wifi_storage/wifi.txt", "network\npassword");
    std::filesystem::create_directory("pf_wifi_storage/config/wifi.txt");
    wifi_text_t out{}; EXPECT_NE(storage_read_wifi_config(&out), ESP_OK); EXPECT_EQ(close_calls, 1);
    std::filesystem::remove_all("pf_wifi_storage/config"); write("pf_wifi_storage/config", "not a directory");
    EXPECT_EQ(storage_read_wifi_config(&out), ESP_FAIL);
}
TEST_F(ReadPicoStorage, EmptyOversizeAndMalformedInputNeverChangeCallerOrName) {
    ASSERT_EQ(storage_init(), ESP_OK);
    wifi_text_t out{}; strcpy(out.ssid, "previous");
    for (const std::string &body : {std::string(""), std::string(1025, 'x'), std::string("ssid\nshort")}) {
        write("pf_wifi_storage/config/wifi.txt", body);
        EXPECT_NE(storage_read_wifi_config(&out), ESP_OK); EXPECT_STREQ(out.ssid, "previous");
    }
    EXPECT_EQ(device_name_calls, 0); EXPECT_EQ(close_calls, 3);
}
TEST_F(ReadPicoStorage, CloseFailureLatchesMediaAndPublishesNoCredentials) {
    ASSERT_EQ(storage_init(), ESP_OK); write("pf_wifi_storage/wifi.txt", "network\npassword\nFrame");
    fail_close = true;
    wifi_text_t out{}; strcpy(out.ssid, "previous");
    EXPECT_NE(storage_read_wifi_config(&out), ESP_OK);
    EXPECT_TRUE(close_failed); EXPECT_STREQ(out.ssid, "previous"); EXPECT_EQ(device_name_calls, 0);
}
TEST_F(ReadPicoStorage, MediaLossAtCloseRejectsOtherwiseValidImport) {
    ASSERT_EQ(storage_init(), ESP_OK); write("pf_wifi_storage/wifi.txt", "network\npassword");
    lose_on_close = true; wifi_text_t out{}; strcpy(out.ssid, "previous");
    EXPECT_EQ(storage_read_wifi_config(&out), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(close_failed); EXPECT_STREQ(out.ssid, "previous");
}
TEST_F(ReadPicoStorage, FullWireCredentialCopyUsesDocumentedCapacitiesAndNoNameSideEffect) {
    ASSERT_EQ(storage_init(), ESP_OK);
    std::string ssid(32, 's'), psk(64, 'f'); write("pf_wifi_storage/wifi.txt", ssid + "\n" + psk);
    char s[33], p[65]; ASSERT_EQ(storage_read_wifi_credentials(s, p), ESP_OK);
    EXPECT_EQ(std::string(s), ssid); EXPECT_EQ(std::string(p), psk); EXPECT_EQ(device_name_calls, 0);
    EXPECT_EQ(storage_read_wifi_config(nullptr), ESP_ERR_INVALID_ARG);
}
