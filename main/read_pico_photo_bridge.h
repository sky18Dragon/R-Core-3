#pragma once
#include <stdint.h>

// Only the isolated diagnostic supplies this display sink. It never refreshes or publishes.
const uint8_t *read_pico_photo_bridge_frame(void);
void read_pico_photo_bridge_release(void);
