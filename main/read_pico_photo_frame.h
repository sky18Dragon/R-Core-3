#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "read_pico_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

// Diagnostic sink for the SAME image_processor rows/columns; no decode or hardware IO.
typedef struct {
    uint8_t *pixels;
    size_t size, next;
    unsigned mode;  // 0 unset, 1 native rows, 2 processing-order native columns
    bool invert_180;
    bool failed;
} photo_frame_t;
bool photo_frame_begin(photo_frame_t *frame, uint8_t *pixels, size_t size, bool invert_180);
bool photo_frame_push(photo_frame_t *frame, bool column, int index, const uint8_t *rgb, int count);
bool photo_frame_complete(const photo_frame_t *frame);

#ifdef __cplusplus
}
#endif
