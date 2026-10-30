#include "sleep_log.h"
#include <string.h>

void sleep_log_reset(sleep_log_t *log)
{
    if (!log) return;
    memset(log, 0, sizeof(*log));
    log->version = SLEEP_LOG_VERSION;
}

bool sleep_log_valid(const sleep_log_t *log)
{
    return log && log->version == SLEEP_LOG_VERSION && log->count <= SLEEP_LOG_DEPTH &&
           log->next < SLEEP_LOG_DEPTH && (log->count == SLEEP_LOG_DEPTH || log->next == log->count);
}

void sleep_log_new_boot(sleep_log_t *log)
{
    if (!log) return;
    log->boot = log->boot == UINT16_MAX ? 1 : (uint16_t) (log->boot + 1);
}

void sleep_log_push(sleep_log_t *log, sleep_event_kind_t kind, uint8_t detail, uint32_t uptime_s,
                    uint32_t arg, int battery_mv)
{
    if (!log) return;
    int16_t mv = battery_mv < 0 ? -1 : battery_mv > INT16_MAX ? INT16_MAX : (int16_t) battery_mv;
    log->entries[log->next] = (sleep_log_entry_t) {
        .boot = log->boot, .kind = (uint8_t) kind, .detail = detail,
        .uptime_s = uptime_s, .arg = arg, .battery_mv = mv,
    };
    log->next = (uint8_t) ((log->next + 1) % SLEEP_LOG_DEPTH);
    if (log->count < SLEEP_LOG_DEPTH) ++log->count;
}

size_t sleep_log_copy(const sleep_log_t *log, sleep_log_entry_t *out, size_t capacity)
{
    if (!log || !out || !sleep_log_valid(log)) return 0;
    size_t n = log->count < capacity ? log->count : capacity;
    unsigned first = (log->next + SLEEP_LOG_DEPTH - log->count) % SLEEP_LOG_DEPTH;
    unsigned skip = log->count - n;  // keep the newest when capacity is short
    for (size_t i = 0; i < n; ++i)
        out[i] = log->entries[(first + skip + i) % SLEEP_LOG_DEPTH];
    return n;
}

const char *sleep_event_name(uint8_t kind)
{
    switch (kind) {
    case SLEEP_EVENT_BOOT: return "boot";
    case SLEEP_EVENT_ATTEMPT: return "sleep_attempt";
    case SLEEP_EVENT_NOT_LATCHED: return "not_latched";
    case SLEEP_EVENT_NOT_CUT: return "latched_power_not_cut";
    }
    return "unknown";
}
