#include <gtest/gtest.h>

#include <climits>

#include "power_gpio_mask.h"

TEST(PowerGpioMask, UnconnectedAndInvalidPinsNeverShift)
{
    for (int pin : {-1, -2, INT_MIN, 64, 65, INT_MAX}) {
        EXPECT_EQ(power_gpio_mask(pin), 0);
    }
}
TEST(PowerGpioMask, EveryRepresentablePinPreservesOriginalBit)
{
    for (int pin = 0; pin < 64; ++pin) {
        EXPECT_EQ(power_gpio_mask(pin), UINT64_C(1) << pin);
    }
}
TEST(PowerGpioMask, ReadPicoNoPhysicalKeysMeansNoWakeMask)
{
    EXPECT_EQ(power_gpio_mask(-1) | power_gpio_mask(-1) | power_gpio_mask(-1), 0);
}
