#pragma once

#include <stdint.h>

// Portable mask construction: NC/negative pins and shifts >=64 yield no bit.
static inline uint64_t power_gpio_mask(int pin)
{
    return pin >= 0 && pin < 64 ? UINT64_C(1) << (unsigned) pin : 0;
}
