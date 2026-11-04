#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "epdiy.h"

// Vendor 16-gray photo sequence (MindReset app_refresh/display.c): flash the panel
// white, then draw the target with GC16 from white. The trimmed default waveform
// has less erase margin, so a direct GC16 differential from the previous photo
// leaves ghosting and makes a gray code's result depend on the old content.
//
// hl->front_fb holds the target on entry and again on return. `saved` is a
// caller-owned scratch frame of `bytes`. When `panel_white` is true the physical
// panel is already white and the flash is skipped. With `wash` the flash is the
// vendor's black/white wash (epd_clear: 3 cycles, as on its boot and lock
// screens) instead of one GC16 pass to white: it clears high-contrast ghosts
// that a single pass leaves, for about a second more. `healthy` is consulted
// after the flash; returning false stops before the second energized transaction.
// Returns the first non-success EpdDrawError, or -1 when `healthy` refused.
int read_pico_white_first_gc16(EpdiyHighlevelState *hl, uint8_t *saved, size_t bytes,
                               bool panel_white, bool wash, int temperature,
                               bool (*healthy)(void));
