#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Portable rules for Read Pico local OTA (image acceptance and boot guard).

// Bytes needed before the image can be judged: image header (24) + first
// segment header (8) + esp_app_desc_t up to and including project_name.
#define OTA_LOCAL_HEADER_BYTES 112
#define OTA_LOCAL_CHIP_ID_ESP32S3 9
#define OTA_LOCAL_PROJECT_NAME "esp32-photoframe"
#define OTA_LOCAL_BOARD_MARKER "+rplf"
#define OTA_LOCAL_MAX_BOOT_TRIES 3

typedef enum {
    OTA_IMAGE_OK,
    OTA_IMAGE_TOO_SHORT,
    OTA_IMAGE_BAD_MAGIC,
    OTA_IMAGE_WRONG_CHIP,
    OTA_IMAGE_NO_APP_DESC,
    OTA_IMAGE_WRONG_PROJECT,
    OTA_IMAGE_WRONG_BOARD,
    OTA_IMAGE_TOO_LARGE,
} ota_image_verdict_t;

// Judge the start of an uploaded image. total_size is the declared upload
// size and must fit slot_size. On OK, version (if non-NULL) receives the
// image's version string (NUL-terminated, at most 31 characters).
ota_image_verdict_t ota_local_check_image(const uint8_t *head, size_t length, size_t total_size,
                                          size_t slot_size, char version[32]);
const char *ota_local_verdict_text(ota_image_verdict_t verdict);

typedef enum {
    OTA_BOOT_NORMAL,      // nothing pending
    OTA_BOOT_TRIAL,       // count this boot as a trial of a new image
    OTA_BOOT_FALL_BACK,   // trial budget used up: boot the previous image
} ota_boot_action_t;

// pending: a new image has not yet been confirmed healthy. tries: trial
// boots already counted (before this one).
ota_boot_action_t ota_local_boot_action(bool running_from_ota_slot, bool pending, uint8_t tries);
#ifdef __cplusplus
}
#endif
