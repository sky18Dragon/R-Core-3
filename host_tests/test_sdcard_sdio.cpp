#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdcard.h"
#include "sdmmc_cmd.h"

extern sdmmc_card_t *card_host;
}

namespace
{
sdmmc_card_t fake_card{0x5d};
esp_err_t mount_result;
esp_err_t unmount_result;
esp_err_t format_result;
bool provide_card;
int mount_calls;
int unmount_calls;
int format_calls;
int print_calls;
std::string mounted_path;
std::string unmounted_path;
std::string formatted_path;
sdmmc_host_t mounted_host;
sdmmc_slot_config_t mounted_slot;
esp_vfs_fat_sdmmc_mount_config_t mounted_config;

sdcard_config_t config(unsigned width)
{
    sdcard_config_t value = {};
    value.mount_point = "/storage";
    value.bus_width = static_cast<uint8_t>(width);
    value.clk_pin = 38;
    value.cmd_pin = 42;
    value.d0_pin = 44;
    value.d1_pin = 1;
    value.d2_pin = 2;
    value.d3_pin = 3;
    return value;
}
}  // namespace

extern "C" const char *esp_err_to_name(esp_err_t)
{
    return "fake";
}

extern "C" esp_err_t esp_vfs_fat_sdmmc_mount(const char *base_path, const sdmmc_host_t *host,
                                             const sdmmc_slot_config_t *slot_config,
                                             const esp_vfs_fat_sdmmc_mount_config_t *mount_config,
                                             sdmmc_card_t **out_card)
{
    ++mount_calls;
    mounted_path = base_path == nullptr ? "" : base_path;
    mounted_host = *host;
    mounted_slot = *slot_config;
    mounted_config = *mount_config;
    *out_card = mount_result == ESP_OK && provide_card ? &fake_card : nullptr;
    return mount_result;
}

extern "C" esp_err_t esp_vfs_fat_sdcard_unmount(const char *base_path, sdmmc_card_t *card)
{
    ++unmount_calls;
    unmounted_path = base_path == nullptr ? "" : base_path;
    EXPECT_EQ(card, &fake_card);
    return unmount_result;
}

extern "C" esp_err_t esp_vfs_fat_sdcard_format(const char *base_path, sdmmc_card_t *card)
{
    ++format_calls;
    formatted_path = base_path == nullptr ? "" : base_path;
    EXPECT_EQ(card, &fake_card);
    return format_result;
}

extern "C" void sdmmc_card_print_info(FILE *, const sdmmc_card_t *card)
{
    ++print_calls;
    EXPECT_EQ(card, &fake_card);
}

class SdcardSdioTest : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        if (card_host != nullptr) {
            unmount_result = ESP_OK;
            EXPECT_EQ(sdcard_deinit(), ESP_OK);
        }
        mount_result = ESP_OK;
        unmount_result = ESP_OK;
        format_result = ESP_OK;
        provide_card = true;
        mount_calls = unmount_calls = format_calls = print_calls = 0;
        mounted_path.clear();
        unmounted_path.clear();
        formatted_path.clear();
        mounted_host = {};
        mounted_slot = {};
        mounted_config = {};
    }

    void TearDown() override
    {
        if (card_host != nullptr) {
            unmount_result = ESP_OK;
            EXPECT_EQ(sdcard_deinit(), ESP_OK);
        }
    }
};

TEST_F(SdcardSdioTest, RejectsInvalidConfigurationWithoutIo)
{
    EXPECT_EQ(sdcard_init(nullptr), ESP_ERR_INVALID_ARG);
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    value.mount_point = nullptr;
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value.mount_point = "";
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value = config(0);
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value = config(2);
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    value.clk_pin = GPIO_NUM_NC;
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value = config(SDCARD_SDMMC_BUS_WIDTH_4);
    value.d2_pin = GPIO_NUM_NC;
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    value.mount_point = "/this/mount/path/is/too/long/for/the/backend";
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(mount_calls, 0);
    EXPECT_FALSE(sdcard_is_mounted());
}

TEST_F(SdcardSdioTest, OneBitModeForcesUnusedDataPinsToNcAndNeverFormats)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    EXPECT_TRUE(sdcard_is_mounted());
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(mounted_path, "/storage");
    EXPECT_EQ(mounted_slot.width, 1);
    EXPECT_EQ(mounted_slot.clk, 38);
    EXPECT_EQ(mounted_slot.cmd, 42);
    EXPECT_EQ(mounted_slot.d0, 44);
    EXPECT_EQ(mounted_slot.d1, GPIO_NUM_NC);
    EXPECT_EQ(mounted_slot.d2, GPIO_NUM_NC);
    EXPECT_EQ(mounted_slot.d3, GPIO_NUM_NC);
    EXPECT_EQ(mounted_slot.flags, 0u);
    EXPECT_EQ(mounted_host.max_freq_khz, SDMMC_FREQ_HIGHSPEED);
    EXPECT_FALSE(mounted_config.format_if_mount_failed);
    EXPECT_EQ(mounted_config.max_files, 5);
    EXPECT_EQ(mounted_config.allocation_unit_size, 16U * 1024U * 3U);
    EXPECT_EQ(print_calls, 1);
}

TEST_F(SdcardSdioTest, FourBitModeForwardsAllDataPins)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_4);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    EXPECT_EQ(mounted_slot.width, 4);
    EXPECT_EQ(mounted_slot.d0, 44);
    EXPECT_EQ(mounted_slot.d1, 1);
    EXPECT_EQ(mounted_slot.d2, 2);
    EXPECT_EQ(mounted_slot.d3, 3);
}

TEST_F(SdcardSdioTest, InternalPullupsAreExplicitOptIn)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    value.internal_pullups = true;
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    EXPECT_EQ(mounted_slot.flags, SDMMC_SLOT_FLAG_INTERNAL_PULLUP);
}

TEST_F(SdcardSdioTest, MountFailureLeavesNoMountedOrFormattableCard)
{
    mount_result = ESP_ERR_TIMEOUT;
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_TIMEOUT);
    EXPECT_FALSE(sdcard_is_mounted());
    EXPECT_EQ(sdcard_format(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(format_calls, 0);
    EXPECT_EQ(sdcard_deinit(), ESP_OK);
    EXPECT_EQ(unmount_calls, 0);
}

TEST_F(SdcardSdioTest, SuccessWithoutCardHandleFailsClosed)
{
    provide_card = false;
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    EXPECT_EQ(sdcard_init(&value), ESP_FAIL);
    EXPECT_FALSE(sdcard_is_mounted());
    EXPECT_EQ(print_calls, 0);
}

TEST_F(SdcardSdioTest, MountedCardRejectsSecondInitialization)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(mount_calls, 1);
}

TEST_F(SdcardSdioTest, FormatIsExplicitAndUsesTheMountedPath)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    format_result = ESP_FAIL;
    EXPECT_EQ(sdcard_format(), ESP_FAIL);
    EXPECT_EQ(format_calls, 1);
    EXPECT_EQ(formatted_path, "/storage");
    EXPECT_TRUE(sdcard_is_mounted());
}

TEST_F(SdcardSdioTest, DeinitUnmountsTheMountedPathAndClearsState)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_4);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    EXPECT_EQ(sdcard_deinit(), ESP_OK);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(unmounted_path, "/storage");
    EXPECT_FALSE(sdcard_is_mounted());
    EXPECT_EQ(sdcard_deinit(), ESP_OK);
    EXPECT_EQ(unmount_calls, 1);
}

TEST_F(SdcardSdioTest, UnmountErrorInvalidatesHandleAndLatchesUntilReboot)
{
    auto value = config(SDCARD_SDMMC_BUS_WIDTH_1);
    ASSERT_EQ(sdcard_init(&value), ESP_OK);
    unmount_result = ESP_FAIL;
    EXPECT_EQ(sdcard_deinit(), ESP_FAIL);
    EXPECT_FALSE(sdcard_is_mounted());
    EXPECT_EQ(card_host, nullptr);
    EXPECT_EQ(sdcard_deinit(), ESP_FAIL);
    EXPECT_EQ(sdcard_init(&value), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(sdcard_format(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(unmount_calls, 1);
    EXPECT_EQ(mount_calls, 1);
    EXPECT_EQ(format_calls, 0);
}
