#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Boot-time removal of orphaned upload temporaries. Only regular files whose
// leaf is exactly ".rc3-upload-XXXXXXXX-XXXXXXXX.part" (8+8 hex, the
// read_pico_upload naming) directly inside an album directory (root/<album>/)
// are removed. Never descends further, never touches any other name, and
// must run before uploads can start.
typedef struct {
    unsigned removed, failed;
    uint64_t bytes;
    bool truncated;  // stopped at max_removals
} upload_sweep_result_t;

bool upload_sweep_is_temp_name(const char *leaf);
// 0 on success (including nothing to do); -1 if root cannot be opened.
int upload_sweep_orphans(const char *root, unsigned max_removals, upload_sweep_result_t *out);
#ifdef __cplusplus
}
#endif
