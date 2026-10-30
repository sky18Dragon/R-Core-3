#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Portable, bounded MIME subset used by the browser image/thumbnail uploads.
// No filesystem, heap, network, or implicit cleanup; sink errors are terminal.
#define UPLOAD_BOUNDARY_MAX 70
#define UPLOAD_HEADER_MAX 1024
#define UPLOAD_FILENAME_MAX 119
typedef struct {
    void *context;
    bool (*begin)(void *, const char *field, const char *filename);
    bool (*write)(void *, const uint8_t *, size_t);
    bool (*end)(void *);
} upload_stream_sink_t;

typedef struct {
    upload_stream_sink_t sink;
    char marker[UPLOAD_BOUNDARY_MAX + 5];
    size_t marker_size;
    char header[UPLOAD_HEADER_MAX + 1];
    size_t header_size;
    uint8_t pending[UPLOAD_BOUNDARY_MAX + 6];
    size_t pending_size;
    uint8_t output[4096];
    size_t output_size;
    size_t total, limit;
    unsigned state, suffix_size, trailing;
    uint8_t suffix[2];
    unsigned seen;
} upload_stream_t;

bool upload_leaf_safe(const char *name);
bool upload_boundary_from_content_type(const char *type, char *out, size_t capacity);
bool upload_stream_init(upload_stream_t *, const char *boundary, size_t limit,
                        const upload_stream_sink_t *);
bool upload_stream_feed(upload_stream_t *, const void *, size_t);
bool upload_stream_finish(upload_stream_t *);

#ifdef __cplusplus
}
#endif
