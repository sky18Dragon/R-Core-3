#pragma once

#include "read_pico_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    READ_PICO_PATTERN_WHITE = 0,
    READ_PICO_PATTERN_BLACK,
    READ_PICO_PATTERN_MIDGRAY,
    READ_PICO_PATTERN_GRAY16,
    READ_PICO_PATTERN_PARITY,
    READ_PICO_PATTERN_CORNERS,
    READ_PICO_PATTERN_GRAY16_LABELED,
    READ_PICO_PATTERN_GRAY16_VENDOR,
    READ_PICO_PATTERN_GRAY16_PAGE,
    READ_PICO_PATTERN_COUNT,
} read_pico_pattern_t;

// Deterministic 1216x684 PhotoFrame high-nibble-first frame, 0 black / 15 white.
// No allocation or IO. Invalid input leaves the destination unchanged; tail is untouched.
bool read_pico_pattern_render(read_pico_pattern_t pattern, uint8_t *frame, size_t size);
const char *read_pico_pattern_name(read_pico_pattern_t pattern);

#ifdef __cplusplus
}
#endif
