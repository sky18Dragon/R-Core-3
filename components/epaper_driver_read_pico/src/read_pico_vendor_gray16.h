#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Diagnostic-only CPU drawing. Requires initialized 1216x684 landscape EPD geometry.
// No IO, power, rotation, waveform or calibration changes. Keeps the original full-screen
// gray16 geometry to isolate the vendor drawing API, NOT the official Demo's complete UI.
bool read_pico_vendor_gray16_render(uint8_t *frame, size_t size);
bool read_pico_vendor_gray16_page_render(uint8_t *frame, size_t size);
