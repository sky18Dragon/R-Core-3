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
    unsigned active_io; // Open/pending file or directory handles and metadata operations.
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

// A photo refresh (white flash + GC16) holds the board lock for about 2 s;
// SD users (io_begin, get_status) wait past one refresh instead of failing
// mid-rotation. Mount/unmount keep the short board-lock timeout.
#define READ_PICO_SD_IO_LOCK_WAIT_MS 5000

// Reserve while holding the same board lock as mount/unmount, before opening
// any SD object. Unmount refuses active users without invalidating the media.
esp_err_t read_pico_sd_io_begin(void);
// Release only after a known-successful close or a completed metadata operation.
// Atomic/no board I/O; an unmatched release latches the same teardown failure.
void read_pico_sd_io_end(void);

typedef enum {
    READ_PICO_SD_COMMAND_NONE,
    READ_PICO_SD_COMMAND_INVALID,
    READ_PICO_SD_COMMAND_STATUS,
    READ_PICO_SD_COMMAND_MOUNT,
    READ_PICO_SD_COMMAND_FILE_TEST,
    READ_PICO_SD_COMMAND_FILE_CHECK,
    READ_PICO_SD_COMMAND_UPLOAD_TEST,
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
