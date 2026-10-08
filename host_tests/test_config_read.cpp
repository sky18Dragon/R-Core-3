#include <gtest/gtest.h>
#include "config_read.h"
TEST(ConfigRead, OnlyMissingPermitsDefaultAndFirstErrorIsPreserved) {
    config_read_guard_t g{0, 2, 3};
    EXPECT_EQ(config_read_result(&g, 2), 2); EXPECT_EQ(g.error, 0);
    EXPECT_EQ(config_read_result(&g, 5), 5);
    EXPECT_EQ(config_read_result(&g, 6), 6); EXPECT_EQ(g.error, 5);
    EXPECT_EQ(config_read_result(&g, 0), 0); EXPECT_EQ(g.error, 5);
}
TEST(ConfigRead, CompleteNulTerminatedStringIncludingExactCapacity) {
    config_read_guard_t g{0, 2, 3};
    EXPECT_EQ(config_read_string(&g, 0, "", 1, 1), 0);
    EXPECT_EQ(config_read_string(&g, 0, "secret", 7, 7), 0);
    EXPECT_EQ(g.error, 0);
}
TEST(ConfigRead, MissingAndErrorDoNotInspectAnUnwrittenBuffer) {
    config_read_guard_t g{0, 2, 3};
    EXPECT_EQ(config_read_string(&g, 2, nullptr, 0, SIZE_MAX), 2);
    EXPECT_EQ(g.error, 0);
    EXPECT_EQ(config_read_string(&g, 9, nullptr, 0, SIZE_MAX), 9);
    EXPECT_EQ(g.error, 9);
}
TEST(ConfigRead, BadLengthUnterminatedAndEmbeddedNulFailClosed) {
    char nonnul[] = {'a', 'b'}; char embedded[] = {'a', 0, 'b', 0};
    for (int i = 0; i < 5; ++i) {
        config_read_guard_t g{0, 2, 3};
        EXPECT_EQ(config_read_string(&g, 0, i == 0 ? nullptr : i == 3 ? nonnul : embedded,
                                     i == 3 ? 2 : 4, i == 1 ? 0 : i == 2 ? SIZE_MAX :
                                     i == 3 ? 2 : 4), 3);
        EXPECT_EQ(g.error, 3);
    }
}
TEST(ConfigRead, IntegerRangeAndReadErrorsRemainDistinct) {
    config_read_guard_t g{0, 2, 3};
    EXPECT_EQ(config_read_integer(&g, 0, -1, -1, INT32_MAX), 0);
    EXPECT_EQ(config_read_integer(&g, 0, INT32_MAX, -1, INT32_MAX), 0);
    EXPECT_EQ(config_read_integer(&g, 2, -999, 0, 1), 2);
    EXPECT_EQ(config_read_integer(&g, 7, 0, 0, 1), 7); EXPECT_EQ(g.error, 7);
    config_read_guard_t other{0, 2, 3};
    EXPECT_EQ(config_read_integer(&other, 0, 2, 0, 1), 3); EXPECT_EQ(other.error, 3);
}
