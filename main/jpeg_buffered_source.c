#include "jpeg_buffered_source.h"

#include <limits.h>
#include <string.h>

bool jpeg_buffered_source_init(jpeg_buffered_source_t *source,
                               const jpeg_stream_probe_io_t *upstream,
                               uint8_t *buffer, size_t capacity, size_t file_size)
{
    if (!source)
        return false;
    memset(source, 0, sizeof(*source));
    source->failed = true;
    if (!upstream || !upstream->read || !upstream->media_ready || !buffer ||
        !capacity || capacity > INT_MAX || !file_size)
        return false;
    source->upstream = *upstream;
    source->buffer = buffer;
    source->capacity = capacity;
    source->remaining = file_size;
    source->failed = false;
    return true;
}

bool jpeg_buffered_source_ready(void *context)
{
    jpeg_buffered_source_t *source = context;
    if (!source || source->failed || !source->upstream.media_ready ||
        !source->upstream.read || !source->buffer || !source->capacity)
        return false;
    if (source->offset == source->available &&
        !source->upstream.media_ready(source->upstream.context)) {
        source->failed = true;
        return false;
    }
    return true;
}

int jpeg_buffered_source_read(void *context, uint8_t *data, size_t size)
{
    jpeg_buffered_source_t *source = context;
    if (!data || size > INT_MAX || !jpeg_buffered_source_ready(source))
        return -1;
    size_t total = 0;
    while (total < size && source->remaining) {
        if (!jpeg_buffered_source_ready(source))
            return -1;
        if (source->offset == source->available) {
            size_t request = source->remaining < source->capacity ?
                                 source->remaining : source->capacity;
            ++source->read_calls;
            int got = source->upstream.read(source->upstream.context, source->buffer, request);
            // Premature EOF is an error, not a successful decode with missing tail.
            if (got <= 0 || (size_t) got > request ||
                !source->upstream.media_ready(source->upstream.context)) {
                source->failed = true;
                return -1;
            }
            source->physical_bytes += (size_t) got;
            source->offset = 0;
            source->available = (size_t) got;
        }
        size_t take = source->available - source->offset;
        if (take > size - total)
            take = size - total;
        memcpy(data + total, source->buffer + source->offset, take);
        source->offset += take;
        source->remaining -= take;
        source->delivered += take;
        total += take;
    }
    return (int) total;
}
