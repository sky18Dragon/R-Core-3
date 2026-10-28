#include <gtest/gtest.h>

#include <cstring>
#include <vector>

extern "C" {
#include "ota_local.h"
}

namespace
{
std::vector<uint8_t> image(const char *version = "v1.0+rplf", const char *project = "esp32-photoframe",
                           uint16_t chip = OTA_LOCAL_CHIP_ID_ESP32S3)
{
    std::vector<uint8_t> b(4096, 0);
    b[0] = 0xE9;
    b[12] = chip & 0xff;
    b[13] = chip >> 8;
    const uint32_t magic = 0xABCD5432u;
    for (int i = 0; i < 4; ++i) b[32 + i] = (magic >> (8 * i)) & 0xff;
    std::strncpy(reinterpret_cast<char *>(&b[32 + 16]), version, 32);
    std::strncpy(reinterpret_cast<char *>(&b[32 + 48]), project, 32);
    return b;
}
ota_image_verdict_t check(const std::vector<uint8_t> &b, size_t total = 2000000, size_t slot = 0x300000)
{
    return ota_local_check_image(b.data(), b.size(), total, slot, nullptr);
}
}  // namespace

TEST(OtaLocalImage, AcceptsMarkedLocalFrameImageAndReturnsVersion)
{
    auto b = image("dev-87c632d+rplf");
    char version[32] = "unchanged";
    EXPECT_EQ(ota_local_check_image(b.data(), b.size(), 1958864, 0x300000, version), OTA_IMAGE_OK);
    EXPECT_STREQ(version, "dev-87c632d+rplf");
}

TEST(OtaLocalImage, RejectsEachMismatch)
{
    auto b = image();
    EXPECT_EQ(ota_local_check_image(b.data(), OTA_LOCAL_HEADER_BYTES - 1, 2000000, 0x300000, nullptr),
              OTA_IMAGE_TOO_SHORT);
    EXPECT_EQ(check(b, 0x300001), OTA_IMAGE_TOO_LARGE);
    EXPECT_EQ(check(b, 10), OTA_IMAGE_TOO_SHORT);
    auto bad = b; bad[0] = 0x00;
    EXPECT_EQ(check(bad), OTA_IMAGE_BAD_MAGIC);
    EXPECT_EQ(check(image("v1+rplf", "esp32-photoframe", 0)), OTA_IMAGE_WRONG_CHIP);  // ESP32
    bad = b; bad[32] ^= 1;
    EXPECT_EQ(check(bad), OTA_IMAGE_NO_APP_DESC);
    EXPECT_EQ(check(image("v1+rplf", "other-project")), OTA_IMAGE_WRONG_PROJECT);
    EXPECT_EQ(check(image("v1+rplf", "esp32-photoframe-x")), OTA_IMAGE_WRONG_PROJECT);
    EXPECT_EQ(check(image("v1.0")), OTA_IMAGE_WRONG_BOARD);       // upstream / other profile
    EXPECT_EQ(check(image("v1+rplf-x")), OTA_IMAGE_WRONG_BOARD);  // marker must end the version
    bad = b; std::memset(&bad[32 + 16], 'a', 32);                 // unterminated version
    EXPECT_EQ(check(bad), OTA_IMAGE_WRONG_BOARD);
    EXPECT_EQ(ota_local_check_image(nullptr, 4096, 2000000, 0x300000, nullptr), OTA_IMAGE_TOO_SHORT);
    EXPECT_STREQ(ota_local_verdict_text(OTA_IMAGE_WRONG_BOARD), "image is not a Read Pico local-frame build");
}

TEST(OtaLocalBoot, TrialsThenFallsBackOnlyForPendingOtaImage)
{
    EXPECT_EQ(ota_local_boot_action(false, true, 9), OTA_BOOT_NORMAL);  // factory never falls back
    EXPECT_EQ(ota_local_boot_action(true, false, 9), OTA_BOOT_NORMAL);  // confirmed image
    for (uint8_t tries = 0; tries < OTA_LOCAL_MAX_BOOT_TRIES; ++tries)
        EXPECT_EQ(ota_local_boot_action(true, true, tries), OTA_BOOT_TRIAL);
    EXPECT_EQ(ota_local_boot_action(true, true, OTA_LOCAL_MAX_BOOT_TRIES), OTA_BOOT_FALL_BACK);
}
