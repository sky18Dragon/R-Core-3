#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *context;
    bool (*media_ready)(void *context);
    // Return bytes read, zero at EOF, negative on error. Never seek past EOF.
    int (*read)(void *context, uint8_t *data, size_t size);
} jpeg_stream_probe_io_t;

typedef enum {
    JPEG_STREAM_OK,
    JPEG_STREAM_INVALID,
    JPEG_STREAM_MEDIA_FAILED,
    JPEG_STREAM_READ_FAILED,
    JPEG_STREAM_PREPARE_FAILED,
    JPEG_STREAM_DIMENSIONS_FAILED,
    JPEG_STREAM_DECODE_FAILED,
} jpeg_stream_probe_error_t;

typedef struct {
    jpeg_stream_probe_error_t error;
    int decoder_result;
    unsigned width, height, output_width, output_height;
    size_t bytes_read, pixels, blocks;
    uint32_t rgb_crc32;  // Decoder block order, NOT a row-major framebuffer hash.
} jpeg_stream_probe_result_t;

// Decode only: no framebuffer, scaling/cropping/dither/EXIF/display or file writes.
// Caller owns/open/closes the source. Work must be suitably aligned, >=3100 bytes.
jpeg_stream_probe_result_t jpeg_stream_probe_run(const jpeg_stream_probe_io_t *io, void *work,
                                                 size_t work_size, unsigned scale);
// Product decoder: same bounded input, copy tiles into caller-owned RGB raster.
// No CRC/EXIF/cropping/dither/display. RGB size is checked before decompression.
jpeg_stream_probe_result_t jpeg_stream_decode_rgb(const jpeg_stream_probe_io_t *io, void *work,
                                                  size_t work_size, unsigned scale, uint8_t *rgb,
                                                  size_t rgb_size);
const char *jpeg_stream_probe_error_name(jpeg_stream_probe_error_t error);

#ifdef __cplusplus
}
#endif
