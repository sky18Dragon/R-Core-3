#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Read-only classification of an SD card's first sector (and, optionally, the
// first sector of its first partition). Pure functions over caller-owned
// buffers; no IO, no allocation. Used to explain a FatFS "no filesystem" mount
// failure without ever writing or formatting the card.

#define SD_DIAG_SECTOR_BYTES 512
#define SD_DIAG_MBR_ENTRIES 4

typedef enum {
    SD_VBR_NONE = 0,  // not a recognized volume boot record
    SD_VBR_FAT12_16,
    SD_VBR_FAT32,
    SD_VBR_EXFAT,
    SD_VBR_NTFS,
} sd_vbr_kind_t;

typedef enum {
    SD_LAYOUT_NO_SIGNATURE = 0,  // no 0x55AA at 510/511
    SD_LAYOUT_VBR,               // no partition table: a filesystem starts at sector 0
    SD_LAYOUT_MBR,               // MBR with at least one non-empty entry
    SD_LAYOUT_GPT_PROTECTIVE,    // MBR entry type 0xEE (GUID partition map)
    SD_LAYOUT_UNKNOWN,           // signature present, but neither VBR nor valid MBR
} sd_layout_t;

typedef struct {
    sd_layout_t layout;
    sd_vbr_kind_t sector0_vbr;  // meaningful for SD_LAYOUT_VBR
    uint8_t type[SD_DIAG_MBR_ENTRIES];
    uint32_t start_lba[SD_DIAG_MBR_ENTRIES];
    uint32_t sectors[SD_DIAG_MBR_ENTRIES];
    int first_partition;  // index of the first non-empty, non-0xEE entry, or -1
} sd_sector_info_t;

// `sector` must hold SD_DIAG_SECTOR_BYTES. Always fills `out`.
void sd_sector_diag_parse(const uint8_t *sector, sd_sector_info_t *out);

// Classify a sector as a volume boot record. Does not require the 0x55AA tail
// for exFAT/NTFS name matching but does for FAT.
sd_vbr_kind_t sd_sector_diag_vbr_kind(const uint8_t *sector);

// True when `sector` begins a GPT header ("EFI PART").
bool sd_sector_diag_is_gpt_header(const uint8_t *sector);

const char *sd_sector_diag_layout_name(sd_layout_t layout);
const char *sd_sector_diag_vbr_name(sd_vbr_kind_t kind);
const char *sd_sector_diag_part_type_name(uint8_t type);

#ifdef __cplusplus
}
#endif
