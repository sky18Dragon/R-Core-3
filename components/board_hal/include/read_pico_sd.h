#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define READ_PICO_SD_MOUNT_POINT "/storage"

typedef struct {
    bool present;
    bool mounted;
    bool media_valid;
    bool exit_failed;
} read_pico_sd_status_t;

// Sole Read Pico caller of PhotoFrame sdcard_init/deinit. No format or file writes.
// All file consumers must close handles before unmount. No hotplug/background task.
esp_err_t read_pico_sd_mount(void);
esp_err_t read_pico_sd_unmount(void);
// CD read error/removal invalidates mounted media until explicit unmount/remount.
esp_err_t read_pico_sd_get_status(read_pico_sd_status_t *out);
// Uncertain file close blocks all new mounts/normal teardown until reboot.
// Does not retry close, release the lease, or perform board I/O.
void read_pico_sd_note_file_close_failure(void);

typedef enum {
    READ_PICO_SD_COMMAND_NONE,
    READ_PICO_SD_COMMAND_INVALID,
    READ_PICO_SD_COMMAND_STATUS,
    READ_PICO_SD_COMMAND_MOUNT,
    READ_PICO_SD_COMMAND_FILE_TEST,
    READ_PICO_SD_COMMAND_FILE_CHECK,
    READ_PICO_SD_COMMAND_PHOTO_CHECK,
    READ_PICO_SD_COMMAND_STOP,
} read_pico_sd_command_t;

typedef struct {
    char bytes[16];
    size_t length;
    bool rejected;
} read_pico_sd_parser_t;

// Zero-initialized parser; exact ASCII whitelist, LF or CRLF, reject whole bad line.
read_pico_sd_command_t read_pico_sd_parser_feed(read_pico_sd_parser_t *parser, unsigned char byte);

#ifdef __cplusplus
}
#endif
