#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t seconds, microseconds;
    bool initialized;
} sntp_stamp_t;

// Unique, strictly increasing within this boot, even after a wall-clock reset.
// Failure leaves both outputs unchanged. This is reply correlation, not auth.
bool sntp_stamp_next(sntp_stamp_t *state, uint32_t seconds, uint32_t microseconds,
                     uint32_t *out_seconds, uint32_t *out_microseconds);
