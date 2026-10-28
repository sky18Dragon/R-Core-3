#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The adapter must implement exclusive create and a genuinely no-replace
// same-filesystem publish (not plain POSIX rename). Deletion is limited to
// upload_file_discard(): this object's own unpublished temporary file.
typedef struct {
    void *context;
    bool (*media_ready)(void *);
    int (*create_new)(void *, const char *);
    int (*write)(void *, int, const uint8_t *, size_t);
    int (*sync)(void *, int);
    int (*close)(void *, int);
    void (*close_uncertain)(void *);
    int (*publish_no_replace)(void *, const char *, const char *);
    // Optional: remove a temporary path this object created. NULL keeps it.
    int (*remove_temp)(void *, const char *);
} upload_file_io_t;

typedef struct {
    upload_file_io_t io;
    char source[512];  // Bound to the exact exclusively-created source, not an arbitrary publish
                       // path.
    int fd;
    size_t bytes, limit;
    bool failed, ready, published, close_failed;
    bool created;  // create_new succeeded for `source` (this object owns that path)
} upload_file_t;

// Begin only on a fresh/closed object; the caller must not replace a live owner.
bool upload_file_begin(upload_file_t *, const upload_file_io_t *, const char *, size_t);
bool upload_file_write(upload_file_t *, const uint8_t *, size_t);
bool upload_file_finish(upload_file_t *, size_t expected);  // 0: nonempty unknown-length MIME part
void upload_file_abort(upload_file_t *);  // close at most once; retains the temporary path
bool upload_file_publish(upload_file_t *, const char *source, const char *destination);
// After a failed upload: close if still open, then remove the temporary file
// only if this object created it, it closed cleanly and was never published.
// Anything uncertain (failed close, foreign path, no remover) is retained.
bool upload_file_discard(upload_file_t *);

#ifdef __cplusplus
}
#endif
