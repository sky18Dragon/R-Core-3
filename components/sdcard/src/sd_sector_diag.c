#include "sd_sector_diag.h"

#include <string.h>

#define MBR_TABLE_OFFSET 446u
#define MBR_ENTRY_BYTES 16u

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
           ((uint32_t) p[3] << 24);
}

static bool has_signature(const uint8_t *s)
{
    return s[510] == 0x55 && s[511] == 0xAA;
}

static bool plausible_bpb(const uint8_t *s)
{
    uint16_t bytes_per_sector = (uint16_t) (s[11] | (s[12] << 8));
    uint8_t sectors_per_cluster = s[13];
    uint8_t fat_count = s[16];
    bool sector_ok = bytes_per_sector == 512 || bytes_per_sector == 1024 ||
                     bytes_per_sector == 2048 || bytes_per_sector == 4096;
    bool cluster_ok = sectors_per_cluster != 0 && (sectors_per_cluster & (sectors_per_cluster - 1)) == 0;
    return sector_ok && cluster_ok && (fat_count == 1 || fat_count == 2);
}

sd_vbr_kind_t sd_sector_diag_vbr_kind(const uint8_t *sector)
{
    if (memcmp(sector + 3, "EXFAT   ", 8) == 0)
        return SD_VBR_EXFAT;
    if (memcmp(sector + 3, "NTFS    ", 8) == 0)
        return SD_VBR_NTFS;
    if (!has_signature(sector) || !plausible_bpb(sector))
        return SD_VBR_NONE;
    if (memcmp(sector + 82, "FAT32   ", 8) == 0)
        return SD_VBR_FAT32;
    if (memcmp(sector + 54, "FAT", 3) == 0)
        return SD_VBR_FAT12_16;
    return SD_VBR_NONE;
}

bool sd_sector_diag_is_gpt_header(const uint8_t *sector)
{
    return memcmp(sector, "EFI PART", 8) == 0;
}

void sd_sector_diag_parse(const uint8_t *sector, sd_sector_info_t *out)
{
    memset(out, 0, sizeof(*out));
    out->first_partition = -1;
    if (!has_signature(sector)) {
        out->layout = SD_LAYOUT_NO_SIGNATURE;
        return;
    }
    out->sector0_vbr = sd_sector_diag_vbr_kind(sector);
    if (out->sector0_vbr != SD_VBR_NONE) {
        out->layout = SD_LAYOUT_VBR;
        return;
    }
    bool valid = true;
    bool any = false;
    bool gpt = false;
    for (unsigned i = 0; i < SD_DIAG_MBR_ENTRIES; ++i) {
        const uint8_t *entry = sector + MBR_TABLE_OFFSET + i * MBR_ENTRY_BYTES;
        if (entry[0] != 0x00 && entry[0] != 0x80)
            valid = false;
        out->type[i] = entry[4];
        out->start_lba[i] = le32(entry + 8);
        out->sectors[i] = le32(entry + 12);
        if (entry[4] == 0)
            continue;
        any = true;
        if (entry[4] == 0xEE)
            gpt = true;
        else if (out->first_partition < 0)
            out->first_partition = (int) i;
    }
    if (!valid || !any) {
        out->layout = SD_LAYOUT_UNKNOWN;
        out->first_partition = -1;
    } else if (gpt) {
        out->layout = SD_LAYOUT_GPT_PROTECTIVE;
    } else {
        out->layout = SD_LAYOUT_MBR;
    }
}

const char *sd_sector_diag_layout_name(sd_layout_t layout)
{
    switch (layout) {
    case SD_LAYOUT_NO_SIGNATURE: return "no-55AA-signature";
    case SD_LAYOUT_VBR: return "no-partition-table(filesystem at sector 0)";
    case SD_LAYOUT_MBR: return "MBR";
    case SD_LAYOUT_GPT_PROTECTIVE: return "GPT(protective MBR)";
    case SD_LAYOUT_UNKNOWN: return "unrecognized";
    }
    return "unrecognized";
}

const char *sd_sector_diag_vbr_name(sd_vbr_kind_t kind)
{
    switch (kind) {
    case SD_VBR_FAT12_16: return "FAT12/16";
    case SD_VBR_FAT32: return "FAT32";
    case SD_VBR_EXFAT: return "exFAT";
    case SD_VBR_NTFS: return "NTFS";
    case SD_VBR_NONE: break;
    }
    return "none";
}

const char *sd_sector_diag_part_type_name(uint8_t type)
{
    switch (type) {
    case 0x00: return "empty";
    case 0x01: return "FAT12";
    case 0x04:
    case 0x06:
    case 0x0E: return "FAT16";
    case 0x0B:
    case 0x0C: return "FAT32";
    case 0x07: return "NTFS/exFAT";
    case 0x83: return "Linux";
    case 0xEE: return "GPT-protective";
    case 0xAF: return "HFS/APFS";
    }
    return "other";
}
