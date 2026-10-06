#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *context;
    // Exact bounded random read; no writes. False includes short reads.
    bool (*read_at)(void *context, size_t offset, uint8_t *data, size_t size);
    size_t size;
} jpeg_metadata_io_t;

typedef struct {
    unsigned width, height, orientation;  // EXIF 1..8; absent is 1.
} jpeg_metadata_t;

typedef enum {
    JPEG_METADATA_OK,
    JPEG_METADATA_INVALID,
    JPEG_METADATA_READ_FAILED,
    JPEG_METADATA_UNSUPPORTED,
    JPEG_METADATA_LIMIT,
} jpeg_metadata_error_t;

// Baseline SOF0, 8-bit grayscale or three-component JPEG; scans through SOS.
// Parses only IFD0 orientation, never EXIF thumbnails/MakerNotes/GPS.
// Bounded to 32MiB source, 4MiB header, 4096 markers and 4096 IFD entries.
jpeg_metadata_error_t jpeg_metadata_read(const jpeg_metadata_io_t *io, jpeg_metadata_t *out);

// Smallest decoder scale preserving detail while fitting a caller RGB budget.
bool jpeg_metadata_scale(const jpeg_metadata_t *info, size_t budget, unsigned *scale,
                         size_t *rgb_bytes);

// Map an oriented-view pixel back to the stored raster (no rotated allocation).
bool jpeg_orientation_map(unsigned width, unsigned height, unsigned orientation, unsigned x,
                          unsigned y, unsigned *source_x, unsigned *source_y);

#ifdef __cplusplus
}
#endif
