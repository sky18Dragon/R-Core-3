#pragma once
#include "jpeg_stream_probe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JPEG_FILE_PROBE_PATH "/storage/RC3P0001.JPG"
#define JPEG_FILE_PROBE_MAX_SIZE (32u * 1024u * 1024u)

typedef struct { bool attempted, close_failed; } jpeg_file_probe_session_t;
typedef struct {
    void *context;
    bool (*media_ready)(void *);
    int (*open_read)(void *);
    int64_t (*file_size)(void *, int);
    int (*read)(void *, int, uint8_t *, size_t);
    int (*close)(void *, int);
} jpeg_file_probe_io_t;
typedef enum {
    JPEG_FILE_OK, JPEG_FILE_INVALID, JPEG_FILE_ALREADY_ATTEMPTED,
    JPEG_FILE_MEDIA_FAILED, JPEG_FILE_OPEN_FAILED, JPEG_FILE_SIZE_FAILED,
    JPEG_FILE_STREAM_FAILED, JPEG_FILE_CLOSE_FAILED, JPEG_FILE_READ_FAILED,
} jpeg_file_probe_error_t;
typedef struct {
    jpeg_file_probe_error_t error;
    size_t file_size;
    size_t bytes_read;  // Entire source, including trailing data, separate from codec consumption.
    jpeg_stream_probe_result_t stream;
} jpeg_file_probe_result_t;

// No writes, arbitrary paths, retry, mount or teardown. Caller owns work buffer.
jpeg_file_probe_result_t jpeg_file_probe_run(jpeg_file_probe_session_t *session,
                                            const jpeg_file_probe_io_t *io,
                                            void *work, size_t work_size);
const char *jpeg_file_probe_error_name(jpeg_file_probe_error_t error);
#ifdef __cplusplus
}
#endif
