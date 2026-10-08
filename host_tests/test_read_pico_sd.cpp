#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "read_pico_board.h"
#include "read_pico_sd.h"
#include "sdcard.h"
}

namespace
{
int lock_depth, claim_calls, release_calls, mount_calls, unmount_calls, detect_calls;
bool present, mounted, lease;
int remove_at;
esp_err_t lock_error, detect_error, claim_error, mount_error, unmount_error;
sdcard_config_t mounted_config;

read_pico_sd_command_t line(read_pico_sd_parser_t &parser, const std::string &input)
{
    auto result = READ_PICO_SD_COMMAND_NONE;
    for (unsigned char byte : input)
        result = read_pico_sd_parser_feed(&parser, byte);
    return result;
}
}  // namespace

extern "C" {
esp_err_t read_pico_board_lock(uint32_t)
{
    if (lock_error != ESP_OK)
        return lock_error;
    ++lock_depth;
    return ESP_OK;
}
void read_pico_board_unlock(void)
{
    EXPECT_GT(lock_depth, 0);
    --lock_depth;
}
esp_err_t read_pico_board_sd_present(bool *out)
{
    EXPECT_GT(lock_depth, 0);
    ++detect_calls;
    *out = detect_error == ESP_OK && present && detect_calls != remove_at;
    return detect_error;
}
esp_err_t read_pico_board_storage_claim(void)
{
    EXPECT_GT(lock_depth, 0);
    ++claim_calls;
    if (claim_error != ESP_OK)
        return claim_error;
    EXPECT_FALSE(lease);
    lease = true;
    return ESP_OK;
}
esp_err_t read_pico_board_storage_release(void)
{
    EXPECT_GT(lock_depth, 0);
    EXPECT_TRUE(lease);
    EXPECT_FALSE(mounted);
    ++release_calls;
    lease = false;
    return ESP_OK;
}
bool sdcard_is_mounted(void)
{
    EXPECT_GT(lock_depth, 0);
    return mounted;
}
esp_err_t sdcard_init(const sdcard_config_t *config)
{
    EXPECT_GT(lock_depth, 0);
    EXPECT_TRUE(lease);
    ++mount_calls;
    mounted_config = *config;
    mounted = mount_error == ESP_OK;
    return mount_error;
}
esp_err_t sdcard_deinit(void)
{
    EXPECT_GT(lock_depth, 0);
    EXPECT_TRUE(lease);
    ++unmount_calls;
    mounted = false;  // IDF can free the handle even on failure.
    return unmount_error;
}
}

class ReadPicoSdTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        lock_depth = claim_calls = release_calls = mount_calls = unmount_calls = detect_calls = 0;
        remove_at = 0;
        present = true;
        mounted = lease = false;
        lock_error = detect_error = claim_error = mount_error = unmount_error = ESP_OK;
        mounted_config = {};
    }
    void TearDown() override
    {
        if (unmount_error == ESP_OK) {
            mounted = lease;  // Leave externally mounted fake media to its external owner.
            EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
            EXPECT_FALSE(lease);
        }
        EXPECT_EQ(lock_depth, 0);
    }
};

TEST_F(ReadPicoSdTest, AbsentOrUnreadableCardNeverMounts)
{
    present = false;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_NOT_FOUND);
    detect_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(claim_calls, 0);
    EXPECT_EQ(mount_calls, 0);
}

TEST_F(ReadPicoSdTest, FileCloseFailureLatchesAndRetainsLeaseWithoutBackendTeardown)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    read_pico_sd_note_file_close_failure();
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_TRUE(status.mounted && status.exit_failed);
    EXPECT_FALSE(status.media_valid);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(lease && mounted);
    EXPECT_EQ(unmount_calls, 0);
    EXPECT_EQ(release_calls, 0);
    unmount_error = ESP_ERR_INVALID_STATE; // No reset/unlatch API; process ends as reboot.
}

TEST_F(ReadPicoSdTest, IoLeaseBlocksUnmountWithoutInvalidatingMountedMedia)
{
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    ASSERT_EQ(read_pico_sd_io_begin(), ESP_OK);
    ASSERT_EQ(read_pico_sd_io_begin(), ESP_OK);
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_EQ(status.active_io, 2u);
    EXPECT_TRUE(status.media_valid && status.mounted);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(unmount_calls, 0);
    EXPECT_TRUE(lease && mounted);
    read_pico_sd_io_end();
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    read_pico_sd_io_end();
    EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
    EXPECT_EQ(unmount_calls, 1);
}

TEST_F(ReadPicoSdTest, RemovalPreventsNewIoButOutstandingUsersCanDrainBeforeUnmount)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    ASSERT_EQ(read_pico_sd_io_begin(), ESP_OK);
    present = false;
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_ERR_NOT_FOUND);
    present = true;
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    read_pico_sd_io_end();
    EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
    EXPECT_EQ(read_pico_sd_mount(), ESP_OK);
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_OK);
    read_pico_sd_io_end();
}

TEST_F(ReadPicoSdTest, IoLockAndCardDetectionFailuresDoNotAcquirePhantomUsers)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    lock_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_ERR_TIMEOUT);
    lock_error = ESP_OK;
    detect_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_io_begin(), ESP_ERR_TIMEOUT);
    detect_error = ESP_OK;
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_EQ(status.active_io, 0u);
    EXPECT_FALSE(status.media_valid);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
}

TEST_F(ReadPicoSdTest, UnmatchedIoReleaseLatchesInsteadOfWrappingOwnershipCount)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    read_pico_sd_io_end();
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_EQ(status.active_io, 0u);
    EXPECT_TRUE(status.exit_failed);
    EXPECT_FALSE(status.media_valid);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(unmount_calls, 0);
    unmount_error = ESP_ERR_INVALID_STATE;
}

TEST_F(ReadPicoSdTest, MountUsesPhotoFramePathOneBitPinsAndOptInPullups)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    EXPECT_STREQ(mounted_config.mount_point, "/storage");
    EXPECT_EQ(mounted_config.bus_width, 1);
    EXPECT_TRUE(mounted_config.internal_pullups);
    EXPECT_EQ(mounted_config.clk_pin, 38);
    EXPECT_EQ(mounted_config.cmd_pin, 42);
    EXPECT_EQ(mounted_config.d0_pin, 44);
    EXPECT_EQ(mounted_config.d1_pin, GPIO_NUM_NC);
    EXPECT_EQ(mounted_config.d2_pin, GPIO_NUM_NC);
    EXPECT_EQ(mounted_config.d3_pin, GPIO_NUM_NC);
    EXPECT_TRUE(lease);
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 1);
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_TRUE(status.present && status.mounted && status.media_valid);
    EXPECT_FALSE(status.exit_failed);
}

TEST_F(ReadPicoSdTest, MountFailureReleasesLeaseWithoutAutomaticRetry)
{
    mount_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(release_calls, 1);
    EXPECT_FALSE(lease);
    mount_error = ESP_OK;
    EXPECT_EQ(read_pico_sd_mount(), ESP_OK);  // Explicit later request only.
    EXPECT_EQ(mount_calls, 2);
}

TEST_F(ReadPicoSdTest, CardRemovedDuringMountIsCleanedBeforePublish)
{
    remove_at = 2;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_NOT_FOUND);
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(release_calls, 1);
    EXPECT_FALSE(lease);
    EXPECT_FALSE(mounted);
}

TEST_F(ReadPicoSdTest, RemovalInvalidatesUntilExplicitUnmountAndRemount)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    present = false;
    read_pico_sd_status_t status;
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_TRUE(status.mounted);
    EXPECT_FALSE(status.media_valid);
    EXPECT_EQ(unmount_calls, 0);  // No implicit unmount while consumers could be open.
    present = true;
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_FALSE(status.media_valid);
    ASSERT_EQ(read_pico_sd_unmount(), ESP_OK);
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_TRUE(status.media_valid);
}

TEST_F(ReadPicoSdTest, ObservationErrorAlsoInvalidatesMountedMedia)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    read_pico_sd_status_t status;
    detect_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_ERR_TIMEOUT);
    EXPECT_FALSE(status.present || status.media_valid);
    EXPECT_TRUE(status.mounted);
    detect_error = ESP_OK;
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_FALSE(status.media_valid);
}

TEST_F(ReadPicoSdTest, RefusesForeignMountAndDoesNotUnmountIt)
{
    mounted = true;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 0);
    EXPECT_EQ(unmount_calls, 0);
    mounted = false;
}

TEST_F(ReadPicoSdTest, LockAndClaimFailuresDoNotStartSdIo)
{
    lock_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_TIMEOUT);
    read_pico_sd_status_t status{true, true, true, true};
    EXPECT_EQ(read_pico_sd_get_status(&status), ESP_ERR_TIMEOUT);
    EXPECT_FALSE(status.present || status.mounted || status.media_valid || status.exit_failed);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_ERR_TIMEOUT);
    lock_error = ESP_OK;
    claim_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 0);
    EXPECT_EQ(read_pico_sd_get_status(nullptr), ESP_ERR_INVALID_ARG);
}

TEST_F(ReadPicoSdTest, UnmountReleasesLeaseOnlyAfterBackendExitAndIsIdempotent)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
    EXPECT_FALSE(mounted || lease);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_OK);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(release_calls, 1);
}

TEST(ReadPicoSdParserTest, AcceptsOnlyExactCommandsAndCrLf)
{
    read_pico_sd_parser_t parser{};
    EXPECT_EQ(line(parser, "status\n"), READ_PICO_SD_COMMAND_STATUS);
    EXPECT_EQ(line(parser, "mount\r\n"), READ_PICO_SD_COMMAND_MOUNT);
    EXPECT_EQ(line(parser, "file-test\r\n"), READ_PICO_SD_COMMAND_FILE_TEST);
    EXPECT_EQ(line(parser, "file-check\r\n"), READ_PICO_SD_COMMAND_FILE_CHECK);
    EXPECT_EQ(line(parser, "upload-test\r\n"), READ_PICO_SD_COMMAND_UPLOAD_TEST);
    EXPECT_EQ(line(parser, "photo-check\r\n"), READ_PICO_SD_COMMAND_PHOTO_CHECK);
    EXPECT_EQ(line(parser, "stop\n"), READ_PICO_SD_COMMAND_STOP);
    for (const char *invalid :
         {"\n", " mount\n", "mount \n", "MOUNT\n", "format\n", "mount\rstop\n", "mount;stop\n",
          "file-test x\n", "file-test /storage/a\n", "FILE-TEST\n", "file-test;stop\n",
          "file-check /storage/a\n", " file-check\n", "file-check;file-test\n", "FILE-CHECK\n",
          "photo-check /storage/a\n", " photo-check\n", "photo-check;stop\n", "PHOTO-CHECK\n"})
        EXPECT_EQ(line(parser, invalid), READ_PICO_SD_COMMAND_INVALID);
}

TEST(ReadPicoSdParserTest, RejectsWholeOverflowBinaryOrControlLineAndRecovers)
{
    read_pico_sd_parser_t parser{};
    for (std::string input : {std::string(300, 'x') + "mount\n", std::string("mount\0", 6) + "\n",
                              std::string("mount\x80\n"), std::string("mount\t\n")}) {
        EXPECT_EQ(line(parser, input), READ_PICO_SD_COMMAND_INVALID);
        EXPECT_EQ(line(parser, "status\n"), READ_PICO_SD_COMMAND_STATUS);
    }
    EXPECT_EQ(line(parser, "mount"), READ_PICO_SD_COMMAND_NONE);
    EXPECT_EQ(read_pico_sd_parser_feed(&parser, '\n'), READ_PICO_SD_COMMAND_MOUNT);
    EXPECT_EQ(read_pico_sd_parser_feed(nullptr, '\n'), READ_PICO_SD_COMMAND_INVALID);
}

// Last in direct whole-suite execution: production latch intentionally has no reset API.
TEST_F(ReadPicoSdTest, UnmountFailureRetainsHalLeaseAndCannotRetryOrRemount)
{
    ASSERT_EQ(read_pico_sd_mount(), ESP_OK);
    unmount_error = ESP_FAIL;
    EXPECT_EQ(read_pico_sd_unmount(), ESP_FAIL);
    EXPECT_TRUE(lease);
    EXPECT_FALSE(mounted);
    EXPECT_EQ(read_pico_sd_unmount(), ESP_FAIL);
    EXPECT_EQ(read_pico_sd_mount(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(release_calls, 0);
    read_pico_sd_status_t status;
    ASSERT_EQ(read_pico_sd_get_status(&status), ESP_OK);
    EXPECT_TRUE(status.exit_failed);
    EXPECT_FALSE(status.mounted || status.media_valid);
}
