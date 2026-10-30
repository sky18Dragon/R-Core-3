#include "image_history.h"
#include <string.h>

void image_history_reset(image_history_t *h)
{
    if (h) memset(h, 0, sizeof(*h));
}

static unsigned slot(const image_history_t *h, unsigned age)
{
    return (h->top + IMAGE_HISTORY_DEPTH - age) % IMAGE_HISTORY_DEPTH;
}

void image_history_push(image_history_t *h, const char *path)
{
    if (!h || !path || !path[0]) return;
    size_t n = strnlen(path, IMAGE_HISTORY_PATH_MAX);
    if (n >= IMAGE_HISTORY_PATH_MAX) return;
    if (h->count && !strcmp(h->paths[h->top], path)) return;
    unsigned next = h->count ? (h->top + 1) % IMAGE_HISTORY_DEPTH : 0;
    memcpy(h->paths[next], path, n + 1);
    h->top = next;
    if (h->count < IMAGE_HISTORY_DEPTH) ++h->count;
}

bool image_history_back(image_history_t *h, char *out, size_t capacity)
{
    if (!h || !out || h->count < 2) return false;
    const char *previous = h->paths[slot(h, 1)];
    size_t n = strlen(previous);
    if (n >= capacity) return false;
    memcpy(out, previous, n + 1);
    h->paths[h->top][0] = '\0';
    h->top = slot(h, 1);
    --h->count;
    return true;
}

void image_history_forget(image_history_t *h, const char *path)
{
    if (!h || !path || !h->count) return;
    // Compact in place, oldest first; also merge neighbours made adjacent.
    unsigned base = slot(h, h->count - 1), kept = 0;
    for (unsigned i = 0; i < h->count; ++i) {
        char *p = h->paths[(base + i) % IMAGE_HISTORY_DEPTH];
        char *last = kept ? h->paths[(base + kept - 1) % IMAGE_HISTORY_DEPTH] : NULL;
        if (!strcmp(p, path) || (last && !strcmp(last, p))) continue;
        char *dest = h->paths[(base + kept) % IMAGE_HISTORY_DEPTH];
        if (dest != p) memcpy(dest, p, strlen(p) + 1);
        ++kept;
    }
    for (unsigned i = kept; i < h->count; ++i) h->paths[(base + i) % IMAGE_HISTORY_DEPTH][0] = '\0';
    h->count = kept;
    h->top = kept ? (base + kept - 1) % IMAGE_HISTORY_DEPTH : 0;
}
