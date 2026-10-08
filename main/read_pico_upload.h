#pragma once
#include "upload_file.h"

#ifdef __cplusplus
extern "C" {
#endif

// Device-only adapter: owned /storage FAT mount; never formats or unlinks.
upload_file_io_t read_pico_upload_io(void);
bool read_pico_upload_begin(upload_file_t *file, const char *directory, char *path, size_t capacity,
                            size_t limit);

#ifdef __cplusplus
}
#endif
