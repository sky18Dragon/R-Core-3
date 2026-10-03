#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define READ_PICO_PANEL_WIDTH 1216u
#define READ_PICO_PANEL_HEIGHT 684u
#define READ_PICO_FRAME_BYTES 415872u

typedef enum {
    READ_PICO_FRAME_OK = 0,
    READ_PICO_FRAME_INVALID_ARGUMENT,
    READ_PICO_FRAME_INVALID_GEOMETRY,
    READ_PICO_FRAME_BUFFER_TOO_SMALL,
    READ_PICO_FRAME_OVERLAP,
} read_pico_frame_error_t;

typedef enum {
    READ_PICO_ROTATE_0 = 0,
    READ_PICO_ROTATE_90,
    READ_PICO_ROTATE_180,
    READ_PICO_ROTATE_270,
} read_pico_rotation_t;

typedef struct {
    size_t width;
    size_t height;
    size_t stride;
} read_pico_frame_layout_t;

// High-nibble-first PhotoFrame -> low-nibble-first EPDiy. Rotation is clockwise.
// 0 = black, 15 = white on both sides; invert is opt-in, never implicit.
// Row padding is not copied/written; an odd destination row's unused nibble is white.
// Reject overlap and invalid layouts before modifying dst. No allocations or board I/O.
read_pico_frame_error_t read_pico_frame_convert(const uint8_t *src, size_t src_size,
                                                read_pico_frame_layout_t src_layout, uint8_t *dst,
                                                size_t dst_size,
                                                read_pico_frame_layout_t dst_layout,
                                                read_pico_rotation_t rotation, bool invert);

#ifdef __cplusplus
}
#endif
