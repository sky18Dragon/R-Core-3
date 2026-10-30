#include "sleep_policy.h"

// Mirrors read_pico_pmu_command.h; kept local so this stays board-agnostic.
#define ALARM_MIN_S 10
#define ALARM_MAX_S 2678400

sleep_decision_t sleep_decide(const sleep_inputs_t *in)
{
    sleep_decision_t no = {false, 0};
    if (!in || !in->enabled || in->power != SLEEP_POWER_BATTERY || in->busy)
        return no;
    int64_t window = in->woke_by_alarm ? SLEEP_AWAKE_AFTER_ALARM_MS : SLEEP_AWAKE_AFTER_KEY_MS;
    if (in->uptime_ms < window)
        return no;
    if (in->last_activity_ms > 0 && in->uptime_ms - in->last_activity_ms < SLEEP_AWAKE_AFTER_ACTIVITY_MS)
        return no;
    if (in->seconds_to_next < 0)
        return (sleep_decision_t) {true, 0};  // nothing scheduled: sleep until a key press
    if (in->seconds_to_next < (int64_t) SLEEP_MIN_SECONDS)
        return no;  // the next rotation is imminent; stay up and do it awake
    int64_t s = in->seconds_to_next;
    if (s > ALARM_MAX_S) s = ALARM_MAX_S;  // wake early and reschedule
    if (s < ALARM_MIN_S) s = ALARM_MIN_S;
    return (sleep_decision_t) {true, (uint32_t) s};
}
