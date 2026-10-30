#include <gtest/gtest.h>

extern "C" {
#include "wifi_watch.h"
}

namespace
{
wifi_watch_t fresh()
{
    wifi_watch_t watch;
    wifi_watch_reset(&watch);
    return watch;
}
}  // namespace

TEST(WifiWatch, ConnectedOrDriverRetryingNeverKicks)
{
    auto watch = fresh();
    for (int64_t t = 0; t < 3600000; t += 1000) {
        EXPECT_FALSE(wifi_watch_step(&watch, t, true, false));
        EXPECT_FALSE(wifi_watch_step(&watch, t, false, false));
    }
    EXPECT_EQ(watch.kicks, 0u);
}

TEST(WifiWatch, StalledLinkKicksAfterFirstDelay)
{
    auto watch = fresh();
    EXPECT_FALSE(wifi_watch_step(&watch, 1000, false, true));
    EXPECT_FALSE(wifi_watch_step(&watch, 1000 + WIFI_WATCH_FIRST_RETRY_MS - 1, false, true));
    EXPECT_TRUE(wifi_watch_step(&watch, 1000 + WIFI_WATCH_FIRST_RETRY_MS, false, true));
    EXPECT_EQ(watch.kicks, 1u);
}

TEST(WifiWatch, BackoffDoublesAndCapsAtMaximum)
{
    auto watch = fresh();
    int64_t now = 0;
    uint32_t expected = WIFI_WATCH_FIRST_RETRY_MS;
    for (int i = 0; i < 12; ++i) {
        ASSERT_FALSE(wifi_watch_step(&watch, now, false, true));  // schedules
        now += expected;
        ASSERT_TRUE(wifi_watch_step(&watch, now, false, true)) << i;
        expected = expected * 2 > WIFI_WATCH_MAX_RETRY_MS ? WIFI_WATCH_MAX_RETRY_MS : expected * 2;
        EXPECT_EQ(watch.backoff_ms, expected);
    }
    EXPECT_EQ(watch.backoff_ms, (uint32_t) WIFI_WATCH_MAX_RETRY_MS);
}

TEST(WifiWatch, ReconnectResetsBackoff)
{
    auto watch = fresh();
    wifi_watch_step(&watch, 0, false, true);
    ASSERT_TRUE(wifi_watch_step(&watch, WIFI_WATCH_FIRST_RETRY_MS, false, true));
    EXPECT_GT(watch.backoff_ms, (uint32_t) WIFI_WATCH_FIRST_RETRY_MS);
    EXPECT_FALSE(wifi_watch_step(&watch, WIFI_WATCH_FIRST_RETRY_MS + 1, true, false));
    EXPECT_EQ(watch.backoff_ms, (uint32_t) WIFI_WATCH_FIRST_RETRY_MS);
    EXPECT_EQ(watch.kicks, 0u);
    EXPECT_FALSE(watch.waiting);
}

TEST(WifiWatch, DriverRetryingKeepsScheduledKick)
{
    auto watch = fresh();
    wifi_watch_step(&watch, 0, false, true);
    // A kicked attempt is in progress (not stalled): no extra kick.
    EXPECT_FALSE(wifi_watch_step(&watch, 10000, false, false));
    EXPECT_TRUE(wifi_watch_step(&watch, WIFI_WATCH_FIRST_RETRY_MS, false, true));
}

TEST(WifiWatch, NullWatchIsIgnored)
{
    EXPECT_FALSE(wifi_watch_step(nullptr, 0, false, true));
}
