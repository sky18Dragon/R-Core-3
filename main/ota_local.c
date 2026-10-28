#include "ota_local.h"
#include <string.h>

#define IMAGE_MAGIC 0xE9
#define IMAGE_CHIP_ID_OFFSET 12
#define APP_DESC_OFFSET 32  // image header (24) + first segment header (8)
#define APP_DESC_MAGIC 0xABCD5432u
#define APP_DESC_VERSION_OFFSET 16
#define APP_DESC_PROJECT_OFFSET 48
#define APP_DESC_TEXT_LEN 32

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
}

static size_t text_length(const uint8_t *p)
{
    size_t n = 0;
    while (n < APP_DESC_TEXT_LEN && p[n]) ++n;
    return n;
}

ota_image_verdict_t ota_local_check_image(const uint8_t *head, size_t length, size_t total_size,
                                          size_t slot_size, char version[32])
{
    if (!head || length < OTA_LOCAL_HEADER_BYTES || total_size < OTA_LOCAL_HEADER_BYTES)
        return OTA_IMAGE_TOO_SHORT;
    if (total_size > slot_size) return OTA_IMAGE_TOO_LARGE;
    if (head[0] != IMAGE_MAGIC) return OTA_IMAGE_BAD_MAGIC;
    uint16_t chip = (uint16_t) (head[IMAGE_CHIP_ID_OFFSET] | head[IMAGE_CHIP_ID_OFFSET + 1] << 8);
    if (chip != OTA_LOCAL_CHIP_ID_ESP32S3) return OTA_IMAGE_WRONG_CHIP;
    const uint8_t *desc = head + APP_DESC_OFFSET;
    if (le32(desc) != APP_DESC_MAGIC) return OTA_IMAGE_NO_APP_DESC;
    const uint8_t *project = desc + APP_DESC_PROJECT_OFFSET;
    size_t project_len = text_length(project);
    if (project_len == APP_DESC_TEXT_LEN || project_len != strlen(OTA_LOCAL_PROJECT_NAME) ||
        memcmp(project, OTA_LOCAL_PROJECT_NAME, project_len))
        return OTA_IMAGE_WRONG_PROJECT;
    const uint8_t *ver = desc + APP_DESC_VERSION_OFFSET;
    size_t ver_len = text_length(ver), marker = strlen(OTA_LOCAL_BOARD_MARKER);
    if (ver_len == APP_DESC_TEXT_LEN || ver_len < marker ||
        memcmp(ver + ver_len - marker, OTA_LOCAL_BOARD_MARKER, marker))
        return OTA_IMAGE_WRONG_BOARD;
    if (version) {
        memcpy(version, ver, ver_len);
        version[ver_len] = '\0';
    }
    return OTA_IMAGE_OK;
}

const char *ota_local_verdict_text(ota_image_verdict_t verdict)
{
    switch (verdict) {
    case OTA_IMAGE_OK: return "ok";
    case OTA_IMAGE_TOO_SHORT: return "image too short";
    case OTA_IMAGE_BAD_MAGIC: return "not an ESP application image";
    case OTA_IMAGE_WRONG_CHIP: return "image is not for ESP32-S3";
    case OTA_IMAGE_NO_APP_DESC: return "image has no application description";
    case OTA_IMAGE_WRONG_PROJECT: return "image is not esp32-photoframe";
    case OTA_IMAGE_WRONG_BOARD: return "image is not a Read Pico local-frame build";
    case OTA_IMAGE_TOO_LARGE: return "image larger than the OTA slot";
    }
    return "unknown";
}

ota_boot_action_t ota_local_boot_action(bool running_from_ota_slot, bool pending, uint8_t tries)
{
    if (!running_from_ota_slot || !pending) return OTA_BOOT_NORMAL;
    return tries >= OTA_LOCAL_MAX_BOOT_TRIES ? OTA_BOOT_FALL_BACK : OTA_BOOT_TRIAL;
}
