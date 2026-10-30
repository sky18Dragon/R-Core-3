#include "upload_asset.h"

#include <stdio.h>
#include <string.h>

static const char prefix[] = ".rc3-upload-";
static bool extension_ok(const char *ext)
{
    return ext && (!strcmp(ext, ".jpg") || !strcmp(ext, ".png") ||
                   !strcmp(ext, ".bmp") || !strcmp(ext, ".epdgz"));
}
static bool identifier(const char *path, const char **suffix)
{
    if (!path || strnlen(path, 512) >= 512)
        return false;
    const char *leaf = strrchr(path, '/');
    if (!leaf || leaf == path)
        return false;
    ++leaf;
    size_t n = sizeof(prefix) - 1;
    if (strncmp(leaf, prefix, n))
        return false;
    leaf += n;
    // Exactly two fixed eight-hex identifiers, never a user filename.
    for (unsigned i = 0; i < 17; ++i) {
        if (i == 8) {
            if (leaf[i] != '-')
                return false;
        } else if (!leaf[i] || !strchr("0123456789abcdef", leaf[i])) {
            return false;
        }
    }
    *suffix = leaf + 17;
    return true;
}
bool upload_asset_is_typed(const char *path)
{
    const char *suffix;
    return identifier(path, &suffix) && extension_ok(suffix);
}
bool upload_asset_preview_path(const char *image, char *out, size_t capacity)
{
    const char *suffix;
    if (!out || !identifier(image, &suffix) || !extension_ok(suffix))
        return false;
    int n = snprintf(out, capacity, "%.*s.preview.jpg", (int)(suffix - image), image);
    return n >= 0 && (size_t)n < capacity;
}
static bool finished(const upload_file_t *file)
{
    const char *suffix;
    return file && file->fd < 0 && file->ready && !file->failed && !file->published &&
           !file->close_failed && file->bytes && identifier(file->source, &suffix) &&
           !strcmp(suffix, ".part");
}
bool upload_asset_plan(const upload_file_t *image, const upload_file_t *preview,
                       const char *extension, upload_asset_t *result)
{
    if (!result)
        return false;
    memset(result, 0, sizeof(*result));
    if (!extension_ok(extension) || !finished(image) ||
        (preview && (!finished(preview) || preview == image ||
                     !strcmp(image->source, preview->source))))
        return false;
    const char *suffix;
    if (!identifier(image->source, &suffix))
        return false;
    int n = snprintf(result->image, sizeof(result->image), "%.*s%s",
                     (int)(suffix - image->source), image->source, extension);
    if (n < 0 || (size_t)n >= sizeof(result->image) ||
        !upload_asset_preview_path(result->image, result->preview, sizeof(result->preview)))
        return false;
    return true;
}
bool upload_asset_publish(upload_file_t *image, upload_file_t *preview,
                          const char *extension, upload_asset_t *result)
{
    if (!upload_asset_plan(image, preview, extension, result))
        return false;
    if (preview) {
        bool ok = upload_file_publish(preview, preview->source, result->preview);
        result->preview_published = preview->published;
        if (!ok)
            return false;
    }
    bool ok = upload_file_publish(image, image->source, result->image);
    result->image_published = image->published;
    return ok;
}
