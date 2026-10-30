#pragma once
#include "upload_file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char image[512];
    char preview[512];
    bool image_published;
    bool preview_published;
} upload_asset_t;

// Requires the exact finished/closed exclusive-create owners, not naked paths.
// Produces immutable typed names and an optional .preview.jpg sidecar. Never
// deletes/replaces old files; partial publication remains visible but unclaimed.
bool upload_asset_publish(upload_file_t *image, upload_file_t *preview,
                          const char *extension, upload_asset_t *result);
bool upload_asset_plan(const upload_file_t *image, const upload_file_t *preview,
                       const char *extension, upload_asset_t *result);
bool upload_asset_is_typed(const char *path);
bool upload_asset_preview_path(const char *image, char *out, size_t capacity);

#ifdef __cplusplus
}
#endif
