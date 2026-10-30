#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Station link supervisor policy for an always-on frame. The event handler
// gives up after a few quick retries; once nothing is in progress ("stalled"),
// the supervisor restarts reconnecting with a capped exponential backoff so a
// router reboot or long outage never leaves the frame offline until power-cycle.
// It never restarts the chip, starts provisioning, or touches credentials.
#define WIFI_WATCH_FIRST_RETRY_MS 30000
#define WIFI_WATCH_MAX_RETRY_MS 600000

typedef struct {
    bool waiting;          // a stalled link has a retry scheduled
    int64_t next_kick_ms;  // when to restart reconnecting
    uint32_t backoff_ms;   // delay before the next scheduled retry
    uint32_t kicks;        // restarts since the link was last up
} wifi_watch_t;

void wifi_watch_reset(wifi_watch_t *watch);
// Call periodically. Returns true when the caller should restart reconnecting
// now. A connected link resets the backoff; a link the driver is still
// retrying (not stalled) is left alone and keeps any scheduled time.
bool wifi_watch_step(wifi_watch_t *watch, int64_t now_ms, bool connected, bool stalled);
#ifdef __cplusplus
}
#endif
