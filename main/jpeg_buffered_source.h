#pragma once

#include "jpeg_stream_probe.h"

// Caller-owned read-ahead adapter. No allocation, seek, close, or media recovery.
// Bounds physical reads to declared length; checks media at cache refill boundaries.
// A removal may be detected up to one cache capacity later. Cached pixels must
// never be published until caller checks complete input/EOF, close, and live media.
typedef struct {
    jpeg_stream_probe_io_t upstream;
    uint8_t *buffer;
    size_t capacity, offset, available, remaining;
    size_t delivered, physical_bytes, read_calls;
    bool failed;
} jpeg_buffered_source_t;

bool jpeg_buffered_source_init(jpeg_buffered_source_t *source,
                               const jpeg_stream_probe_io_t *upstream,
                               uint8_t *buffer, size_t capacity, size_t file_size);
bool jpeg_buffered_source_ready(void *context);
int jpeg_buffered_source_read(void *context, uint8_t *data, size_t size);
