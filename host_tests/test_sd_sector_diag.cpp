#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "sd_sector_diag.h"

namespace {
std::vector<uint8_t> blank() { return std::vector<uint8_t>(SD_DIAG_SECTOR_BYTES, 0); }

void sign(std::vector<uint8_t> &s)
{
    s[510] = 0x55;
    s[511] = 0xAA;
}

void entry(std::vector<uint8_t> &s, int i, uint8_t boot, uint8_t type, uint32_t lba, uint32_t n)
{
    uint8_t *e = s.data() + 446 + 16 * i;
    e[0] = boot;
    e[4] = type;
    for (int b = 0; b < 4; ++b) {
        e[8 + b] = (lba >> (8 * b)) & 0xff;
        e[12 + b] = (n >> (8 * b)) & 0xff;
    }
}

void fat32_vbr(std::vector<uint8_t> &s)
{
    s[0] = 0xEB; s[1] = 0x58; s[2] = 0x90;
    s[11] = 0x00; s[12] = 0x02;  // 512 bytes per sector
    s[13] = 64;                  // sectors per cluster
    s[16] = 2;                   // FATs
    memcpy(&s[82], "FAT32   ", 8);
    sign(s);
}
}  // namespace

TEST(SdSectorDiag, ZeroedSectorHasNoSignature)
{
    auto s = blank();
    sd_sector_info_t info;
    sd_sector_diag_parse(s.data(), &info);
    EXPECT_EQ(info.layout, SD_LAYOUT_NO_SIGNATURE);
    EXPECT_EQ(info.first_partition, -1);
}

TEST(SdSectorDiag, Fat32WithoutPartitionTableIsVbr)
{
    auto s = blank();
    fat32_vbr(s);
    sd_sector_info_t info;
    sd_sector_diag_parse(s.data(), &info);
    EXPECT_EQ(info.layout, SD_LAYOUT_VBR);
    EXPECT_EQ(info.sector0_vbr, SD_VBR_FAT32);
}

TEST(SdSectorDiag, MbrWithFat32PartitionReportsFirstPartition)
{
    auto s = blank();
    sign(s);
    entry(s, 0, 0x00, 0x0C, 8192, 100000);
    sd_sector_info_t info;
    sd_sector_diag_parse(s.data(), &info);
    EXPECT_EQ(info.layout, SD_LAYOUT_MBR);
    EXPECT_EQ(info.first_partition, 0);
    EXPECT_EQ(info.start_lba[0], 8192u);
    EXPECT_EQ(info.sectors[0], 100000u);
    EXPECT_STREQ(sd_sector_diag_part_type_name(info.type[0]), "FAT32");
}

TEST(SdSectorDiag, ProtectiveMbrIsGptAndNotAFirstPartition)
{
    auto s = blank();
    sign(s);
    entry(s, 0, 0x00, 0xEE, 1, 0xFFFFFFFF);
    sd_sector_info_t info;
    sd_sector_diag_parse(s.data(), &info);
    EXPECT_EQ(info.layout, SD_LAYOUT_GPT_PROTECTIVE);
    EXPECT_EQ(info.first_partition, -1);
}

TEST(SdSectorDiag, InvalidBootFlagIsNotTreatedAsMbr)
{
    auto s = blank();
    sign(s);
    entry(s, 0, 0x55, 0x0C, 2048, 1000);
    sd_sector_info_t info;
    sd_sector_diag_parse(s.data(), &info);
    EXPECT_EQ(info.layout, SD_LAYOUT_UNKNOWN);
    EXPECT_EQ(info.first_partition, -1);
}

TEST(SdSectorDiag, ExFatAndNtfsAreRecognizedByName)
{
    auto s = blank();
    memcpy(&s[3], "EXFAT   ", 8);
    EXPECT_EQ(sd_sector_diag_vbr_kind(s.data()), SD_VBR_EXFAT);
    auto n = blank();
    memcpy(&n[3], "NTFS    ", 8);
    EXPECT_EQ(sd_sector_diag_vbr_kind(n.data()), SD_VBR_NTFS);
}

TEST(SdSectorDiag, BootCodeWithNoPlausibleBpbIsNotAVbr)
{
    auto s = blank();
    s[0] = 0xEB;
    s[2] = 0x90;
    memcpy(&s[82], "FAT32   ", 8);  // string alone is not enough without a sane BPB
    sign(s);
    EXPECT_EQ(sd_sector_diag_vbr_kind(s.data()), SD_VBR_NONE);
}

TEST(SdSectorDiag, GptHeaderSignature)
{
    auto s = blank();
    memcpy(s.data(), "EFI PART", 8);
    EXPECT_TRUE(sd_sector_diag_is_gpt_header(s.data()));
    s[0] = 'X';
    EXPECT_FALSE(sd_sector_diag_is_gpt_header(s.data()));
}
