#include <gtest/gtest.h>

extern "C" {
#include "sleep_policy.h"
}

namespace
{
sleep_inputs_t ready()
{
    sleep_inputs_t in{};
    in.enabled = true;
    in.power = SLEEP_POWER_BATTERY;
    in.woke_by_alarm = true;
    in.uptime_ms = SLEEP_AWAKE_AFTER_ALARM_MS;
    in.seconds_to_next = 3600;
    return in;
}
}  // namespace

TEST(SleepPolicy, SleepsOnBatteryUntilNextRotation)
{
    auto in = ready();
    auto d = sleep_decide(&in);
    EXPECT_TRUE(d.sleep);
    EXPECT_EQ(d.alarm_seconds, 3600u);
}

TEST(SleepPolicy, NeverSleepsUnlessDefinitelyOnBatteryAndEnabled)
{
    auto in = ready();
    in.power = SLEEP_POWER_EXTERNAL;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in.power = SLEEP_POWER_UNKNOWN;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in = ready();
    in.enabled = false;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in = ready();
    in.busy = true;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    EXPECT_FALSE(sleep_decide(nullptr).sleep);
}

TEST(SleepPolicy, KeepsAwakeWindowsAndRecentActivity)
{
    auto in = ready();
    in.uptime_ms = SLEEP_AWAKE_AFTER_ALARM_MS - 1;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in.woke_by_alarm = false;  // key wake or cold boot: longer window
    in.uptime_ms = SLEEP_AWAKE_AFTER_KEY_MS - 1;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in.uptime_ms = SLEEP_AWAKE_AFTER_KEY_MS;
    EXPECT_TRUE(sleep_decide(&in).sleep);
    in.last_activity_ms = in.uptime_ms - SLEEP_AWAKE_AFTER_ACTIVITY_MS + 1;
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in.last_activity_ms = in.uptime_ms - SLEEP_AWAKE_AFTER_ACTIVITY_MS;
    EXPECT_TRUE(sleep_decide(&in).sleep);
}

TEST(SleepPolicy, AlarmRangeAndSchedulingEdges)
{
    auto in = ready();
    in.seconds_to_next = SLEEP_MIN_SECONDS - 1;  // imminent: rotate awake first
    EXPECT_FALSE(sleep_decide(&in).sleep);
    in.seconds_to_next = SLEEP_MIN_SECONDS;
    EXPECT_EQ(sleep_decide(&in).alarm_seconds, SLEEP_MIN_SECONDS);
    in.seconds_to_next = 90LL * 24 * 3600;  // beyond the PMU's 31-day limit
    EXPECT_EQ(sleep_decide(&in).alarm_seconds, 2678400u);
    in.seconds_to_next = -1;  // no schedule: key wake only
    auto d = sleep_decide(&in);
    EXPECT_TRUE(d.sleep);
    EXPECT_EQ(d.alarm_seconds, 0u);
}
