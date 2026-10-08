#pragma once
#include "upload_file.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    void *context;
    // Positive bytes, zero EOF, negative error. Never return more than capacity.
    int (*read)(void *context, uint8_t *buffer, size_t capacity);
    int (*close)(void *context);
    void (*yield)(void *context); // Optional scheduler handoff at 32 KiB boundaries.
} upload_copy_source_t;
// Owns closing the already-open source once, on every path. Copies to an
// exclusive destination owner with bounded stack scratch, exact length + EOF,
// checked close and sync. Neither source nor destination is deleted on failure.
bool upload_copy_finish(upload_file_t *destination, const upload_copy_source_t *source,
                        size_t expected);
#ifdef __cplusplus
}
#endif
