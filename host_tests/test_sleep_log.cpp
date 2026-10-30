#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "sleep_log.h"
}

TEST(SleepLog, KeepsEventsInOrderAcrossBoots)
{
    sleep_log_t log;
    sleep_log_reset(&log);
    EXPECT_TRUE(sleep_log_valid(&log));
    sleep_log_new_boot(&log);
    sleep_log_push(&log, SLEEP_EVENT_BOOT, 2, 0, 1, 4100);
    sleep_log_push(&log, SLEEP_EVENT_ATTEMPT, 0, 320, 180, 4090);
    sleep_log_new_boot(&log);
    sleep_log_push(&log, SLEEP_EVENT_BOOT, 6, 0, 1, -5);
    sleep_log_entry_t out[SLEEP_LOG_DEPTH];
    ASSERT_EQ(sleep_log_copy(&log, out, SLEEP_LOG_DEPTH), 3u);
    EXPECT_EQ(out[0].boot, 1);
    EXPECT_EQ(out[0].detail, 2);
    EXPECT_EQ(out[1].kind, SLEEP_EVENT_ATTEMPT);
    EXPECT_EQ(out[1].arg, 180u);
    EXPECT_EQ(out[1].uptime_s, 320u);
    EXPECT_EQ(out[2].boot, 2);
    EXPECT_EQ(out[2].detail, 6);  // alarm wake
    EXPECT_EQ(out[2].battery_mv, -1);
    EXPECT_STREQ(sleep_event_name(SLEEP_EVENT_NOT_CUT), "latched_power_not_cut");
    EXPECT_STREQ(sleep_event_name(99), "unknown");
}

TEST(SleepLog, FullRingDropsOldestAndShortCopyKeepsNewest)
{
    sleep_log_t log;
    sleep_log_reset(&log);
    for (uint32_t i = 0; i < SLEEP_LOG_DEPTH + 5; ++i)
        sleep_log_push(&log, SLEEP_EVENT_ATTEMPT, 0, i, i, 4000);
    EXPECT_EQ(log.count, SLEEP_LOG_DEPTH);
    EXPECT_TRUE(sleep_log_valid(&log));
    sleep_log_entry_t out[SLEEP_LOG_DEPTH];
    ASSERT_EQ(sleep_log_copy(&log, out, SLEEP_LOG_DEPTH), (size_t) SLEEP_LOG_DEPTH);
    EXPECT_EQ(out[0].arg, 5u);
    EXPECT_EQ(out[SLEEP_LOG_DEPTH - 1].arg, SLEEP_LOG_DEPTH + 4u);
    ASSERT_EQ(sleep_log_copy(&log, out, 3), 3u);
    EXPECT_EQ(out[0].arg, SLEEP_LOG_DEPTH + 2u);
    EXPECT_EQ(out[2].arg, SLEEP_LOG_DEPTH + 4u);
}

TEST(SleepLog, RejectsCorruptStoredBlobs)
{
    sleep_log_t log;
    sleep_log_reset(&log);
    sleep_log_push(&log, SLEEP_EVENT_BOOT, 0, 0, 0, 0);
    sleep_log_t bad = log;
    bad.version = 0;
    EXPECT_FALSE(sleep_log_valid(&bad));
    bad = log;
    bad.count = SLEEP_LOG_DEPTH + 1;
    EXPECT_FALSE(sleep_log_valid(&bad));
    bad = log;
    bad.next = 5;  // not full, so next must equal count
    EXPECT_FALSE(sleep_log_valid(&bad));
    sleep_log_entry_t out[1];
    EXPECT_EQ(sleep_log_copy(&bad, out, 1), 0u);
    EXPECT_FALSE(sleep_log_valid(nullptr));
}

TEST(SleepLog, BootCounterWrapsToOne)
{
    sleep_log_t log;
    sleep_log_reset(&log);
    log.boot = UINT16_MAX;
    sleep_log_new_boot(&log);
    EXPECT_EQ(log.boot, 1);
}
