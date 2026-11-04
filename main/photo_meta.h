#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "cJSON.h"
#ifdef __cplusplus
extern "C" {
#endif
// Per-photo facts the image files cannot carry (docs/PRD-PHOTO-ORIENTATION.md):
// which way is up and when the photo was added. One small index per album,
// "<album>/.photos.json" = {"name.epdgz": {"up": 90, "added": 1793700000}}.
// A photo missing from the index is a landscape photo (up 0) of unknown age, so
// a lost or unreadable index only returns the frame to its old behaviour.
//
// `up` is how far the stored frame must turn clockwise to stand upright, in
// degrees: 0 / 180 are landscape photos, 90 / 270 portrait photos (stored
// turned on their side to fill the panel).
#define PHOTO_META_FILENAME ".photos.json"
#define PHOTO_META_MAX_BYTES (64 * 1024)

bool photo_meta_up_valid(int up);
// Up after an upload turned the source `rotation` degrees clockwise to fill the
// panel. A quarter turn of a portrait source is undone by turning back; any other
// turn was a correction, so the stored photo is already upright.
int photo_meta_up_for_upload(int rotation, bool portrait_source);
// Extra clockwise turn that shows a photo upright on a frame turned `device`
// degrees clockwise from its normal landscape position: 0 or 180 fit, 90 or 270
// mean the photo does not suit the frame's orientation.
int photo_meta_turn(int up, int device);

// In-memory index. photo_meta_parse never returns NULL for valid input sizes:
// unreadable text gives an empty index.
cJSON *photo_meta_parse(const char *text);
int photo_meta_up(const cJSON *index, const char *name);
int64_t photo_meta_added(const cJSON *index, const char *name);
// Sets `up`, keeping `added` when the photo is already known; false on bad input.
bool photo_meta_set(cJSON *index, const char *name, int up, int64_t added_if_new);
void photo_meta_forget(cJSON *index, const char *name);

// Album directory store. Saving writes ".photos.json.new", keeps the previous
// index as ".photos.json.old" while swapping, then removes it; loading falls
// back to ".old" when an interrupted save left no current file.
cJSON *photo_meta_load(const char *album_dir);
bool photo_meta_save(const char *album_dir, const cJSON *index);
// Up of an image given by its full path ("<album dir>/<name>"); 0 when unknown.
int photo_meta_lookup_up(const char *image_path);

// Turns a packed 4-bit frame (two pixels per byte, high nibble first, even width)
// half a turn in place.
void photo_meta_rotate180_4bpp(uint8_t *frame, size_t bytes);
#ifdef __cplusplus
}
#endif
