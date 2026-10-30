#pragma once
#include "upload_file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPLOAD_PROBE_STAGE "/storage/.RC3U0001.PART"
#define UPLOAD_PROBE_CONFLICT "/storage/.RC3U0002.PART"
#define UPLOAD_PROBE_TARGET "/storage/RC3U0001.BIN"
typedef struct {
    upload_file_io_t file;
    bool (*target_absent)(void *);
    int (*open_read)(void *, const char *);
    int (*read)(void *, int, uint8_t *, size_t);
} upload_probe_io_t;
typedef struct {
    bool attempted, close_failed;
} upload_probe_session_t;
typedef struct {
    bool ok, conflict_rejected;
    unsigned created, published;
    size_t written, verified;
    const char *stage;
} upload_probe_result_t;
// Once per startup; exclusively creates two synthetic fixed files, publishes
// one, verifies twice, and requires an EEXIST no-replace conflict. Never deletes.
upload_probe_result_t upload_probe_run(upload_probe_session_t *, const upload_probe_io_t *);

#ifdef __cplusplus
}
#endif
