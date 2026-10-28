#include "wifi_watch.h"
#include <string.h>

void wifi_watch_reset(wifi_watch_t *watch)
{
    memset(watch, 0, sizeof(*watch));
    watch->backoff_ms = WIFI_WATCH_FIRST_RETRY_MS;
}

bool wifi_watch_step(wifi_watch_t *watch, int64_t now_ms, bool connected, bool stalled)
{
    if (!watch) return false;
    if (connected) {
        wifi_watch_reset(watch);
        return false;
    }
    if (!stalled) return false;
    if (!watch->waiting) {
        if (watch->backoff_ms == 0) watch->backoff_ms = WIFI_WATCH_FIRST_RETRY_MS;
        watch->waiting = true;
        watch->next_kick_ms = now_ms + watch->backoff_ms;
        return false;
    }
    if (now_ms < watch->next_kick_ms) return false;
    watch->waiting = false;
    ++watch->kicks;
    uint32_t next = watch->backoff_ms * 2;
    watch->backoff_ms = next > WIFI_WATCH_MAX_RETRY_MS || next < watch->backoff_ms
                            ? WIFI_WATCH_MAX_RETRY_MS
                            : next;
    return true;
}
