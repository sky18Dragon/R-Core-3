#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_FILE_PROBE_SIZE 4096u
#define SD_FILE_PROBE_CHUNK 256u
#define SD_FILE_PROBE_PATH "/storage/RC3T0001.BIN"

// Portable diagnostic only. No path supplied by commands, remove or overwrite API.
typedef struct {
    void *context;
    bool (*media_ready)(void *context);
    int (*open_new)(void *context);  // Must exclusively create the fixed path.
    int (*open_read)(void *context);
    int (*write)(void *context, int fd, const uint8_t *data, size_t size);
    int (*read)(void *context, int fd, uint8_t *data, size_t size);
    int (*sync)(void *context, int fd);
    int (*close)(void *context, int fd);  // Never retried on error.
} sd_file_probe_io_t;

// Read-only capability: no create/write/sync callbacks, no arbitrary path.
typedef struct {
    void *context;
    bool (*media_ready)(void *context);
    int (*open_read)(void *context);
    int (*read)(void *context, int fd, uint8_t *data, size_t size);
    int (*close)(void *context, int fd);
} sd_file_probe_read_io_t;

typedef enum {
    SD_FILE_PROBE_OK,
    SD_FILE_PROBE_INVALID,
    SD_FILE_PROBE_ALREADY_ATTEMPTED,
    SD_FILE_PROBE_MEDIA_UNAVAILABLE,
    SD_FILE_PROBE_CREATE_FAILED,
    SD_FILE_PROBE_WRITE_FAILED,
    SD_FILE_PROBE_SYNC_FAILED,
    SD_FILE_PROBE_CLOSE_FAILED,
    SD_FILE_PROBE_REOPEN_FAILED,
    SD_FILE_PROBE_READ_FAILED,
    SD_FILE_PROBE_MISMATCH,
    SD_FILE_PROBE_EXTRA_DATA,
    SD_FILE_PROBE_READ_OPEN_FAILED,
} sd_file_probe_error_t;

typedef struct {
    bool attempted;
    bool check_attempted;
    bool close_failed;  // Ambiguous descriptor lifetime: block unmount/HAL exit.
} sd_file_probe_session_t;

typedef struct {
    sd_file_probe_error_t error;
    bool created;
    size_t written;
    size_t verified;
} sd_file_probe_result_t;

uint8_t sd_file_probe_byte(size_t offset);
// Once per zero-initialized session, including failures. Keeps created file.
sd_file_probe_result_t sd_file_probe_run(sd_file_probe_session_t *session,
                                       const sd_file_probe_io_t *io);
// Once per session, independently of file-test; checks existing fixed file only.
// Never repairs missing/corrupt data. Shares the close-failure teardown latch.
sd_file_probe_result_t sd_file_probe_check(sd_file_probe_session_t *session,
                                         const sd_file_probe_read_io_t *io);
const char *sd_file_probe_error_name(sd_file_probe_error_t error);

#ifdef __cplusplus
}
#endif
