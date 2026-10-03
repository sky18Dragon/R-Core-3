#include "read_pico_frame.h"

#include <string.h>

static read_pico_frame_error_t span(read_pico_frame_layout_t layout, size_t size, size_t *used)
{
    if (layout.width == 0 || layout.height == 0)
        return READ_PICO_FRAME_INVALID_GEOMETRY;
    size_t row = layout.width / 2 + layout.width % 2;
    if (layout.stride < row || layout.height - 1 > (SIZE_MAX - row) / layout.stride)
        return READ_PICO_FRAME_INVALID_GEOMETRY;
    *used = (layout.height - 1) * layout.stride + row;
    return size < *used ? READ_PICO_FRAME_BUFFER_TOO_SMALL : READ_PICO_FRAME_OK;
}

read_pico_frame_error_t read_pico_frame_convert(const uint8_t *src, size_t src_size,
                                                read_pico_frame_layout_t source, uint8_t *dst,
                                                size_t dst_size, read_pico_frame_layout_t target,
                                                read_pico_rotation_t rotation, bool invert)
{
    if (src == NULL || dst == NULL || rotation < READ_PICO_ROTATE_0 ||
        rotation > READ_PICO_ROTATE_270)
        return READ_PICO_FRAME_INVALID_ARGUMENT;
    size_t source_span, target_span;
    read_pico_frame_error_t err = span(source, src_size, &source_span);
    if (err != READ_PICO_FRAME_OK)
        return err;
    err = span(target, dst_size, &target_span);
    if (err != READ_PICO_FRAME_OK)
        return err;
    bool quarter_turn = rotation == READ_PICO_ROTATE_90 || rotation == READ_PICO_ROTATE_270;
    if (target.width != (quarter_turn ? source.height : source.width) ||
        target.height != (quarter_turn ? source.width : source.height))
        return READ_PICO_FRAME_INVALID_GEOMETRY;

    // Compare half-open address spans without overflowing an end pointer.
    uintptr_t a = (uintptr_t) src, b = (uintptr_t) dst;
    if (source_span > UINTPTR_MAX - a || target_span > UINTPTR_MAX - b)
        return READ_PICO_FRAME_INVALID_ARGUMENT;
    if ((a <= b && b - a < source_span) || (b < a && a - b < target_span))
        return READ_PICO_FRAME_OVERLAP;

    size_t row_bytes = target.width / 2 + target.width % 2;
    if (rotation == READ_PICO_ROTATE_0 && !invert && source.width % 2 == 0) {
        for (size_t y = 0; y < source.height; ++y) {
            for (size_t x = 0; x < row_bytes; ++x) {
                uint8_t value = src[y * source.stride + x];
                dst[y * target.stride + x] = (uint8_t) ((value << 4) | (value >> 4));
            }
        }
        return READ_PICO_FRAME_OK;
    }
    for (size_t y = 0; y < target.height; ++y)
        memset(dst + y * target.stride, 0xff, row_bytes);
    for (size_t y = 0; y < source.height; ++y) {
        for (size_t x = 0; x < source.width; ++x) {
            uint8_t packed = src[y * source.stride + x / 2];
            uint8_t value = x % 2 ? packed & 15 : packed >> 4;
            if (invert)
                value = 15 - value;
            size_t dx = x, dy = y;
            switch (rotation) {
            case READ_PICO_ROTATE_90:
                dx = source.height - 1 - y;
                dy = x;
                break;
            case READ_PICO_ROTATE_180:
                dx = source.width - 1 - x;
                dy = source.height - 1 - y;
                break;
            case READ_PICO_ROTATE_270:
                dx = y;
                dy = source.width - 1 - x;
                break;
            default:
                break;
            }
            uint8_t *pixel = dst + dy * target.stride + dx / 2;
            if (dx % 2)
                *pixel = (uint8_t) ((*pixel & 15) | (value << 4));
            else
                *pixel = (uint8_t) ((*pixel & 0xf0) | value);
        }
    }
    return READ_PICO_FRAME_OK;
}
