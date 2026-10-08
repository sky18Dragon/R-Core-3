#include "sntp_stamp.h"
#include <stddef.h>

bool sntp_stamp_next(sntp_stamp_t *state, uint32_t seconds, uint32_t microseconds,
                     uint32_t *out_seconds, uint32_t *out_microseconds)
{
    if (!state || !out_seconds || !out_microseconds || microseconds >= 1000000 ||
        (state->initialized && state->microseconds >= 1000000)) return false;
    if (state->initialized && (seconds < state->seconds ||
        (seconds == state->seconds && microseconds <= state->microseconds))) {
        seconds = state->seconds;
        microseconds = state->microseconds;
        if (microseconds == 999999) {
            if (seconds == UINT32_MAX) return false;
            ++seconds; microseconds = 0;
        } else {
            ++microseconds;
        }
    }
    state->seconds = *out_seconds = seconds;
    state->microseconds = *out_microseconds = microseconds;
    state->initialized = true;
    return true;
}
