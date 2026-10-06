#include <gtest/gtest.h>

extern "C" {
#include "read_pico_sd.h"
#include "storage.h"
}

namespace
{
int mount_calls, unmount_calls, fallback_calls, device_name_calls;
esp_err_t mount_result, unmount_result, status_result;
read_pico_sd_status_t status;
}  // namespace

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
   protected:
    void SetUp() override
    {
        unmount_result = ESP_OK;
        ASSERT_EQ(storage_unmount_checked(), ESP_OK);
        mount_calls = unmount_calls = fallback_calls = device_name_calls = 0;
        mount_result = status_result = ESP_OK;
        status = {true, true, true, false};
    }
    void TearDown() override
    {
        unmount_result = ESP_OK;
        EXPECT_EQ(storage_unmount_checked(), ESP_OK);
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
    EXPECT_EQ(storage_read_wifi_credentials(ssid, password), ESP_ERR_NOT_FOUND);
    ASSERT_EQ(storage_init(), ESP_OK);
    status.media_valid = false;
    EXPECT_EQ(storage_read_wifi_credentials(ssid, password), ESP_ERR_NOT_FOUND);
    EXPECT_STREQ(ssid, "preserved");
    EXPECT_STREQ(password, "preserved");
    EXPECT_EQ(device_name_calls, 0);
}
