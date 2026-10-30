#pragma once
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Bounded in-RAM history of displayed image paths for "previous image".
// Not persisted; the caller serializes access (display_manager's mutex).
#define IMAGE_HISTORY_DEPTH 16
#define IMAGE_HISTORY_PATH_MAX 256

typedef struct {
    char paths[IMAGE_HISTORY_DEPTH][IMAGE_HISTORY_PATH_MAX];
    unsigned count;  // valid entries
    unsigned top;    // index of the newest entry when count > 0
} image_history_t;

void image_history_reset(image_history_t *history);
// Record a displayed path. Re-showing the newest entry is not a new step;
// a full history drops its oldest entry. Empty or oversized paths are ignored.
void image_history_push(image_history_t *history, const char *path);
// Step back: drop the newest entry and copy the one before it to out, which
// becomes the newest. False (history unchanged) with fewer than two entries
// or when out is too small.
bool image_history_back(image_history_t *history, char *out, size_t capacity);
// Remove every entry equal to path (e.g. a deleted file), keeping order.
void image_history_forget(image_history_t *history, const char *path);
#ifdef __cplusplus
}
#endif
