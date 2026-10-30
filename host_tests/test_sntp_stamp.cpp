#include <gtest/gtest.h>
extern "C" {
#include "sntp_stamp.h"
}
TEST(SntpStamp, FirstAndForwardTimeArePreserved) {
    sntp_stamp_t state{}; uint32_t seconds = 9, us = 9;
    EXPECT_TRUE(sntp_stamp_next(&state, 100, 500, &seconds, &us));
    EXPECT_EQ(seconds, 100u); EXPECT_EQ(us, 500u);
    EXPECT_TRUE(sntp_stamp_next(&state, 101, 0, &seconds, &us)); EXPECT_EQ(seconds, 101u); EXPECT_EQ(us, 0u);
}
TEST(SntpStamp, RepeatedAndBackwardTimeRemainStrictlyUnique) {
    sntp_stamp_t state{100, 500, true}; uint32_t seconds, us;
    EXPECT_TRUE(sntp_stamp_next(&state, 100, 500, &seconds, &us)); EXPECT_EQ(us, 501u);
    EXPECT_TRUE(sntp_stamp_next(&state, 0, 0, &seconds, &us)); EXPECT_EQ(seconds, 100u); EXPECT_EQ(us, 502u);
}
TEST(SntpStamp, FractionOverflowCarriesWithoutRepeatingTimestamp) {
    sntp_stamp_t state{100, 999999, true}; uint32_t seconds, us;
    EXPECT_TRUE(sntp_stamp_next(&state, 1, 0, &seconds, &us)); EXPECT_EQ(seconds, 101u); EXPECT_EQ(us, 0u);
}
TEST(SntpStamp, ExhaustionRetainsStateAndOutputs) {
    sntp_stamp_t state{UINT32_MAX, 999999, true}; uint32_t seconds = 7, us = 8;
    EXPECT_FALSE(sntp_stamp_next(&state, 1, 0, &seconds, &us)); EXPECT_EQ(seconds, 7u); EXPECT_EQ(us, 8u);
    EXPECT_EQ(state.seconds, UINT32_MAX); EXPECT_EQ(state.microseconds, 999999u);
}
TEST(SntpStamp, NullInvalidInputAndInvalidStateDoNotPublish) {
    sntp_stamp_t state{}; uint32_t seconds = 7, us = 8;
    EXPECT_FALSE(sntp_stamp_next(nullptr, 1, 0, &seconds, &us));
    EXPECT_FALSE(sntp_stamp_next(&state, 1, 0, nullptr, &us));
    EXPECT_FALSE(sntp_stamp_next(&state, 1, 0, &seconds, nullptr));
    EXPECT_FALSE(sntp_stamp_next(&state, 1, 1000000, &seconds, &us));
    EXPECT_FALSE(state.initialized); state = {1, 1000000, true};
    EXPECT_FALSE(sntp_stamp_next(&state, 2, 0, &seconds, &us)); EXPECT_EQ(seconds, 7u); EXPECT_EQ(us, 8u);
}
