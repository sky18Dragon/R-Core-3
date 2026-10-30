#include "read_pico_photo_frame.h"

#include <string.h>

bool photo_frame_begin(photo_frame_t *f, uint8_t *pixels, size_t size, bool invert_180)
{
    if (!f || !pixels || size != READ_PICO_FRAME_BYTES)
        return false;
    *f = (photo_frame_t) {.pixels = pixels, .size = size, .invert_180 = invert_180};
    memset(pixels, 0xff, size);
    return true;
}

bool photo_frame_push(photo_frame_t *f, bool column, int index, const uint8_t *rgb, int count)
{
    if (!f || !f->pixels || f->size != READ_PICO_FRAME_BYTES || f->failed)
        return false;
    unsigned mode = column ? 2 : 1;
    size_t lines = column ? READ_PICO_PANEL_WIDTH : READ_PICO_PANEL_HEIGHT;
    int length = column ? READ_PICO_PANEL_HEIGHT : READ_PICO_PANEL_WIDTH;
    int expected = column ? (int) READ_PICO_PANEL_WIDTH - 1 - (int) f->next : (int) f->next;
    if (!rgb || f->next >= lines || index != expected || count != length ||
        (f->mode && f->mode != mode)) {
        f->failed = true;
        return false;
    }
    // Validate the complete line before mutating the 4bpp destination.
    for (int i = 0; i < count; ++i)
        if (rgb[3 * i] != rgb[3 * i + 1] || rgb[3 * i] != rgb[3 * i + 2] || rgb[3 * i] % 17) {
            f->failed = true;
            return false;
    }
    for (int i = 0; i < count; ++i) {
        size_t x = column ? (size_t) index : (size_t) i;
        size_t y = column ? (size_t) i : (size_t) index;
        if (f->invert_180) {
            x = READ_PICO_PANEL_WIDTH - 1 - x;
            y = READ_PICO_PANEL_HEIGHT - 1 - y;
        }
        size_t offset = (y * READ_PICO_PANEL_WIDTH + x) / 2;
        uint8_t gray = rgb[3 * i] / 17;
        unsigned shift = (x & 1) ? 0 : 4;
        f->pixels[offset] = (f->pixels[offset] & ~(15u << shift)) | (gray << shift);
    }
    f->mode = mode;
    ++f->next;
    return true;
}

bool photo_frame_complete(const photo_frame_t *f)
{
    return f && f->pixels && f->size == READ_PICO_FRAME_BYTES && !f->failed &&
           ((f->mode == 1 && f->next == READ_PICO_PANEL_HEIGHT) ||
            (f->mode == 2 && f->next == READ_PICO_PANEL_WIDTH));
}
