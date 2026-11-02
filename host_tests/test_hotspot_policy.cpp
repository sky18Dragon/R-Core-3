#include <gtest/gtest.h>

#include <string>
#include <vector>

extern "C" {
#include "hotspot_policy.h"
}

namespace
{
constexpr int64_t kMinute = 60 * 1000;

hotspot_policy_t fresh(uint32_t idle_minutes = HOTSPOT_IDLE_DEFAULT_MIN)
{
    hotspot_policy_t policy;
    hotspot_policy_init(&policy, idle_minutes);
    return policy;
}

// Request on at t=0 and confirm the start; returns the time it went on.
int64_t turn_on(hotspot_policy_t &policy)
{
    EXPECT_TRUE(hotspot_policy_request(&policy, true, 0));
    EXPECT_EQ(hotspot_policy_step(&policy, HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_START);
    hotspot_policy_started(&policy, true, HOTSPOT_REQUEST_DELAY_MS);
    EXPECT_EQ(policy.state, HOTSPOT_ON);
    return HOTSPOT_REQUEST_DELAY_MS;
}
}  // namespace

TEST(HotspotPolicy, RequestWaitsForTheResponseDelay)
{
    auto policy = fresh();
    EXPECT_TRUE(hotspot_policy_request(&policy, true, 1000));
    EXPECT_TRUE(hotspot_policy_wants_on(&policy));
    EXPECT_EQ(hotspot_policy_step(&policy, 1000 + HOTSPOT_REQUEST_DELAY_MS - 1, 0, false),
              HOTSPOT_STEP_NONE);
    EXPECT_EQ(hotspot_policy_step(&policy, 1000 + HOTSPOT_REQUEST_DELAY_MS, 0, false),
              HOTSPOT_STEP_START);
    EXPECT_EQ(policy.state, HOTSPOT_STARTING);
    // Nothing else is issued until the owner reports the result.
    EXPECT_EQ(hotspot_policy_step(&policy, 10 * kMinute, 0, false), HOTSPOT_STEP_NONE);
}

TEST(HotspotPolicy, ToggleBackBeforeTheDelayDoesNothing)
{
    auto policy = fresh();
    hotspot_policy_request(&policy, true, 0);
    hotspot_policy_request(&policy, false, 100);
    EXPECT_FALSE(hotspot_policy_wants_on(&policy));
    EXPECT_EQ(hotspot_policy_step(&policy, kMinute, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_EQ(policy.state, HOTSPOT_OFF);
    EXPECT_FALSE(policy.pending);
}

TEST(HotspotPolicy, UserStopIsDelayedAndRecorded)
{
    auto policy = fresh();
    int64_t on = turn_on(policy);
    EXPECT_TRUE(hotspot_policy_request(&policy, false, on));
    EXPECT_EQ(hotspot_policy_step(&policy, on + HOTSPOT_REQUEST_DELAY_MS, 1, false),
              HOTSPOT_STEP_STOP);
    hotspot_policy_stopped(&policy, true);
    EXPECT_EQ(policy.state, HOTSPOT_OFF);
    EXPECT_EQ(policy.last_stop_reason, HOTSPOT_REASON_USER);
}

TEST(HotspotPolicy, StopWaitsForUploadToFinish)
{
    auto policy = fresh();
    int64_t on = turn_on(policy);
    hotspot_policy_request(&policy, false, on);
    int64_t due = on + HOTSPOT_REQUEST_DELAY_MS;
    EXPECT_EQ(hotspot_policy_step(&policy, due, 1, true), HOTSPOT_STEP_NONE);
    EXPECT_EQ(hotspot_policy_step(&policy, due + kMinute, 1, true), HOTSPOT_STEP_NONE);
    EXPECT_EQ(policy.state, HOTSPOT_ON);
    EXPECT_EQ(hotspot_policy_step(&policy, due + kMinute + 1, 1, false), HOTSPOT_STEP_STOP);
}

TEST(HotspotPolicy, ClosesAfterIdleLimitWithoutClients)
{
    auto policy = fresh(10);
    int64_t on = turn_on(policy);
    EXPECT_EQ(hotspot_policy_remaining_ms(&policy, on), 10 * kMinute);
    EXPECT_EQ(hotspot_policy_step(&policy, on + 10 * kMinute - 1, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_EQ(hotspot_policy_step(&policy, on + 10 * kMinute, 0, false), HOTSPOT_STEP_STOP);
    hotspot_policy_stopped(&policy, true);
    EXPECT_EQ(policy.last_stop_reason, HOTSPOT_REASON_IDLE);
}

TEST(HotspotPolicy, ClientsAndUploadsKeepItOpen)
{
    auto policy = fresh(10);
    int64_t on = turn_on(policy);
    int64_t t = on;
    for (; t < on + 30 * kMinute; t += 1000) {
        EXPECT_EQ(hotspot_policy_step(&policy, t, 1, false), HOTSPOT_STEP_NONE);
    }
    for (int64_t end = t + 30 * kMinute; t < end; t += 1000) {
        EXPECT_EQ(hotspot_policy_step(&policy, t, 0, true), HOTSPOT_STEP_NONE);
    }
    // The idle clock restarts when the last client leaves.
    EXPECT_EQ(hotspot_policy_step(&policy, t + 10 * kMinute - 1001, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_EQ(hotspot_policy_step(&policy, t + 10 * kMinute, 0, false), HOTSPOT_STEP_STOP);
}

TEST(HotspotPolicy, NeverCloseWhenLimitIsZero)
{
    auto policy = fresh(0);
    int64_t on = turn_on(policy);
    EXPECT_EQ(hotspot_policy_remaining_ms(&policy, on), -1);
    EXPECT_EQ(hotspot_policy_step(&policy, on + 24 * 60 * kMinute, 0, false), HOTSPOT_STEP_NONE);
}

TEST(HotspotPolicy, NewIdleLimitCountsFromNow)
{
    auto policy = fresh(10);
    int64_t on = turn_on(policy);
    hotspot_policy_set_idle_minutes(&policy, 30, on + 9 * kMinute);
    EXPECT_EQ(hotspot_policy_step(&policy, on + 11 * kMinute, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_EQ(hotspot_policy_step(&policy, on + 39 * kMinute, 0, false), HOTSPOT_STEP_STOP);
}

TEST(HotspotPolicy, FailedStartCanBeRetried)
{
    auto policy = fresh();
    hotspot_policy_request(&policy, true, 0);
    ASSERT_EQ(hotspot_policy_step(&policy, HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_START);
    hotspot_policy_started(&policy, false, HOTSPOT_REQUEST_DELAY_MS);
    EXPECT_EQ(policy.state, HOTSPOT_FAILED);
    EXPECT_FALSE(hotspot_policy_wants_on(&policy));
    EXPECT_EQ(hotspot_policy_step(&policy, kMinute, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_TRUE(hotspot_policy_request(&policy, true, kMinute));
    EXPECT_EQ(hotspot_policy_step(&policy, kMinute + HOTSPOT_REQUEST_DELAY_MS, 0, false),
              HOTSPOT_STEP_START);
}

TEST(HotspotPolicy, UnconfirmedStopRefusesStartAndRetriesStop)
{
    auto policy = fresh();
    int64_t on = turn_on(policy);
    hotspot_policy_request(&policy, false, on);
    ASSERT_EQ(hotspot_policy_step(&policy, on + HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_STOP);
    hotspot_policy_stopped(&policy, false);
    EXPECT_EQ(policy.state, HOTSPOT_FAILED);
    EXPECT_TRUE(policy.stop_unconfirmed);
    EXPECT_FALSE(hotspot_policy_request(&policy, true, kMinute));
    // No automatic retry loop: only an explicit request retries the stop.
    EXPECT_EQ(hotspot_policy_step(&policy, 2 * kMinute, 0, false), HOTSPOT_STEP_NONE);
    EXPECT_TRUE(hotspot_policy_request(&policy, false, 2 * kMinute));
    EXPECT_EQ(hotspot_policy_step(&policy, 2 * kMinute + HOTSPOT_REQUEST_DELAY_MS, 0, false),
              HOTSPOT_STEP_STOP);
    hotspot_policy_stopped(&policy, true);
    EXPECT_EQ(policy.state, HOTSPOT_OFF);
    EXPECT_FALSE(policy.stop_unconfirmed);
}

TEST(HotspotPolicy, StopRequestedWhileStartingRunsAfterStart)
{
    auto policy = fresh();
    hotspot_policy_request(&policy, true, 0);
    ASSERT_EQ(hotspot_policy_step(&policy, HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_START);
    hotspot_policy_request(&policy, false, HOTSPOT_REQUEST_DELAY_MS + 10);
    hotspot_policy_started(&policy, true, HOTSPOT_REQUEST_DELAY_MS + 20);
    EXPECT_EQ(hotspot_policy_step(&policy, 2 * HOTSPOT_REQUEST_DELAY_MS + 10, 0, false),
              HOTSPOT_STEP_STOP);
}

TEST(HotspotPolicy, OwnsRadioOnlyWhileHeldOrUnconfirmed)
{
    auto policy = fresh();
    EXPECT_FALSE(hotspot_policy_owns_radio(&policy));
    hotspot_policy_request(&policy, true, 0);
    EXPECT_TRUE(hotspot_policy_owns_radio(&policy));  // about to take it
    ASSERT_EQ(hotspot_policy_step(&policy, HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_START);
    EXPECT_TRUE(hotspot_policy_owns_radio(&policy));
    hotspot_policy_started(&policy, false, HOTSPOT_REQUEST_DELAY_MS);
    EXPECT_FALSE(hotspot_policy_owns_radio(&policy));  // failed start: station recovery may act
    int64_t on = turn_on(policy);
    EXPECT_TRUE(hotspot_policy_owns_radio(&policy));
    hotspot_policy_request(&policy, false, on);
    ASSERT_EQ(hotspot_policy_step(&policy, on + HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_STOP);
    EXPECT_TRUE(hotspot_policy_owns_radio(&policy));
    hotspot_policy_stopped(&policy, false);
    EXPECT_TRUE(hotspot_policy_owns_radio(&policy));  // exit unconfirmed
    hotspot_policy_request(&policy, false, kMinute);
    ASSERT_EQ(hotspot_policy_step(&policy, kMinute + HOTSPOT_REQUEST_DELAY_MS, 0, false), HOTSPOT_STEP_STOP);
    hotspot_policy_stopped(&policy, true);
    EXPECT_FALSE(hotspot_policy_owns_radio(&policy));
}

TEST(HotspotPolicy, IdleMinutesChoices)
{
    for (uint32_t m : {10u, 30u, 60u}) {
        EXPECT_TRUE(hotspot_idle_minutes_valid(m, false));
        EXPECT_TRUE(hotspot_idle_minutes_valid(m, true));
    }
    EXPECT_TRUE(hotspot_idle_minutes_valid(0, false));
    EXPECT_FALSE(hotspot_idle_minutes_valid(0, true));
    EXPECT_FALSE(hotspot_idle_minutes_valid(5, false));
    EXPECT_FALSE(hotspot_idle_minutes_valid(120, false));
}

TEST(HotspotPassword, GeneratedFromAlphabetWithoutLookAlikes)
{
    std::vector<uint8_t> bytes(64);
    for (size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<uint8_t>(i * 37 + 11);
    char out[HOTSPOT_PASSWORD_LEN + 1];
    ASSERT_TRUE(hotspot_password_generate(bytes.data(), bytes.size(), out));
    EXPECT_EQ(std::string(out).size(), static_cast<size_t>(HOTSPOT_PASSWORD_LEN));
    for (char c : std::string(out)) {
        EXPECT_NE(std::string(HOTSPOT_PASSWORD_ALPHABET).find(c), std::string::npos);
        EXPECT_EQ(std::string("0O1IL").find(c), std::string::npos);
    }
    EXPECT_TRUE(hotspot_password_valid(out));
}

TEST(HotspotPassword, RejectsBiasedTailAndShortInput)
{
    const size_t size = sizeof(HOTSPOT_PASSWORD_ALPHABET) - 1;
    const uint8_t biased = static_cast<uint8_t>(256 - 256 % size);  // first rejected value
    std::vector<uint8_t> bytes(HOTSPOT_PASSWORD_LEN, biased);
    char out[HOTSPOT_PASSWORD_LEN + 1] = "unchang";
    EXPECT_FALSE(hotspot_password_generate(bytes.data(), bytes.size(), out));
    EXPECT_STREQ(out, "unchang");
    bytes.assign(HOTSPOT_PASSWORD_LEN - 1, 0);
    EXPECT_FALSE(hotspot_password_generate(bytes.data(), bytes.size(), out));
    // Rejected bytes are skipped, not mapped.
    bytes = {biased, 0, 1, 2, 3, 4, 5, 6, 7};
    ASSERT_TRUE(hotspot_password_generate(bytes.data(), bytes.size(), out));
    EXPECT_STREQ(out, "ABCDEFGH");
}

TEST(HotspotPassword, Validation)
{
    EXPECT_TRUE(hotspot_password_valid("12345678"));
    EXPECT_TRUE(hotspot_password_valid(std::string(63, 'a').c_str()));
    EXPECT_FALSE(hotspot_password_valid("1234567"));
    EXPECT_FALSE(hotspot_password_valid(std::string(64, 'a').c_str()));
    EXPECT_FALSE(hotspot_password_valid("abc\tdefgh"));
    EXPECT_FALSE(hotspot_password_valid("abcdefg\x80"));
    EXPECT_FALSE(hotspot_password_valid(nullptr));
}

TEST(HotspotSsid, Validation)
{
    EXPECT_TRUE(hotspot_ssid_valid("PhotoFrame - 3F2A1"));
    EXPECT_TRUE(hotspot_ssid_valid(std::string(32, 'x').c_str()));
    EXPECT_TRUE(hotspot_ssid_valid("相框"));  // UTF-8 bytes are allowed
    EXPECT_FALSE(hotspot_ssid_valid(""));
    EXPECT_FALSE(hotspot_ssid_valid(std::string(33, 'x').c_str()));
    EXPECT_FALSE(hotspot_ssid_valid("a\nb"));
    EXPECT_FALSE(hotspot_ssid_valid("a\x7f"));
}

TEST(HotspotQr, EscapesSpecialCharacters)
{
    char out[160];
    ASSERT_TRUE(hotspot_wifi_qr("PhotoFrame - 3F2A1", "K7MP4TQX", out, sizeof(out)));
    EXPECT_STREQ(out, "WIFI:T:WPA;S:PhotoFrame - 3F2A1;P:K7MP4TQX;;");
    ASSERT_TRUE(hotspot_wifi_qr("a;b,c:d", "p\\q\"r;s:t", out, sizeof(out)));
    EXPECT_STREQ(out, "WIFI:T:WPA;S:a\\;b\\,c\\:d;P:p\\\\q\\\"r\\;s\\:t;;");
}

TEST(HotspotQr, CapacityAndInvalidInputLeaveOutputUnchanged)
{
    char small[20] = "same";
    EXPECT_FALSE(hotspot_wifi_qr("PhotoFrame - 3F2A1", "K7MP4TQX", small, sizeof(small)));
    EXPECT_STREQ(small, "same");
    char out[160] = "same";
    EXPECT_FALSE(hotspot_wifi_qr("", "K7MP4TQX", out, sizeof(out)));
    EXPECT_FALSE(hotspot_wifi_qr("ssid", "short", out, sizeof(out)));
    EXPECT_STREQ(out, "same");
    // Worst case: every character escaped at maximum lengths still fits its buffer.
    std::string ssid(32, ';'), password(63, ';');
    char big[256];
    ASSERT_TRUE(hotspot_wifi_qr(ssid.c_str(), password.c_str(), big, sizeof(big)));
    EXPECT_EQ(std::string(big).size(), 13u + 64u + 3u + 126u + 2u);
}

TEST(HotspotClock, Range)
{
    EXPECT_FALSE(hotspot_clock_valid(0));
    EXPECT_FALSE(hotspot_clock_valid(HOTSPOT_CLOCK_MIN - 1));
    EXPECT_TRUE(hotspot_clock_valid(HOTSPOT_CLOCK_MIN));
    EXPECT_TRUE(hotspot_clock_valid(1793577600));  // 2026-11-01
    EXPECT_FALSE(hotspot_clock_valid(HOTSPOT_CLOCK_MAX));
}

TEST(HotspotRetry, WaitsFirstDelayThenBacksOffToTheCap)
{
    hotspot_retry_t retry;
    hotspot_retry_start(&retry, 0);
    EXPECT_EQ(hotspot_retry_remaining_ms(&retry, 0), HOTSPOT_RETRY_FIRST_MS);
    EXPECT_FALSE(hotspot_retry_step(&retry, HOTSPOT_RETRY_FIRST_MS - 1, 0, false));
    int64_t now = HOTSPOT_RETRY_FIRST_MS;
    ASSERT_TRUE(hotspot_retry_step(&retry, now, 0, false));
    uint32_t expected = 2 * HOTSPOT_RETRY_FIRST_MS;
    for (int i = 0; i < 8; ++i) {
        hotspot_retry_failed(&retry, now);
        EXPECT_EQ(hotspot_retry_remaining_ms(&retry, now), expected);
        EXPECT_FALSE(hotspot_retry_step(&retry, now + expected - 1, 0, false));
        now += expected;
        ASSERT_TRUE(hotspot_retry_step(&retry, now, 0, false));
        expected = expected * 2 > HOTSPOT_RETRY_MAX_MS ? HOTSPOT_RETRY_MAX_MS : expected * 2;
    }
    EXPECT_EQ(retry.attempts, 9u);
}

TEST(HotspotRetry, NeverRunsUnderAPhoneOrUpload)
{
    hotspot_retry_t retry;
    hotspot_retry_start(&retry, 0);
    int64_t t = 0;
    for (; t < 60 * kMinute; t += 1000) EXPECT_FALSE(hotspot_retry_step(&retry, t, 1, false));
    for (int64_t end = t + 10 * kMinute; t < end; t += 1000)
        EXPECT_FALSE(hotspot_retry_step(&retry, t, 0, true));
    // After the last one leaves it waits the first delay again.
    EXPECT_FALSE(hotspot_retry_step(&retry, t, 0, false));
    EXPECT_TRUE(hotspot_retry_step(&retry, t - 1000 + HOTSPOT_RETRY_FIRST_MS, 0, false));
}

TEST(HotspotRetry, StoppedRetryDoesNothing)
{
    hotspot_retry_t retry;
    hotspot_retry_start(&retry, 0);
    hotspot_retry_stop(&retry);
    EXPECT_FALSE(hotspot_retry_step(&retry, 10 * kMinute, 0, false));
    hotspot_retry_failed(&retry, 10 * kMinute);
    EXPECT_FALSE(retry.armed);
    EXPECT_EQ(hotspot_retry_remaining_ms(&retry, 0), -1);
}
