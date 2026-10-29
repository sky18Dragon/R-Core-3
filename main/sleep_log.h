#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Small persistent ring of battery-sleep events, so a test on battery (no
// USB serial) can be explained afterwards. Portable; the caller stores the
// whole struct as one NVS blob and writes only when an event is pushed.
#define SLEEP_LOG_VERSION 1
#define SLEEP_LOG_DEPTH 24

typedef enum {
    SLEEP_EVENT_BOOT = 1,         // detail: PMU wake reason, arg: ESP reset reason
    SLEEP_EVENT_ATTEMPT = 2,      // arg: alarm seconds (0 = key wake only)
    SLEEP_EVENT_NOT_LATCHED = 3,  // detail: low byte of the ESP error
    SLEEP_EVENT_NOT_CUT = 4,      // PMU latched SOFT_SLEEP but power stayed on
} sleep_event_kind_t;

typedef struct {
    uint16_t boot;       // boot counter when the event happened
    uint8_t kind;        // sleep_event_kind_t
    uint8_t detail;
    uint32_t uptime_s;
    uint32_t arg;
    int16_t battery_mv;  // -1 unknown
    uint16_t reserved;
} sleep_log_entry_t;

typedef struct {
    uint8_t version;
    uint8_t count;
    uint8_t next;
    uint8_t reserved;
    uint16_t boot;
    uint16_t reserved2;
    sleep_log_entry_t entries[SLEEP_LOG_DEPTH];
} sleep_log_t;

void sleep_log_reset(sleep_log_t *log);
// A blob read back from storage is usable only if its header is consistent.
bool sleep_log_valid(const sleep_log_t *log);
// Count a new boot (wraps at 65535 -> 1).
void sleep_log_new_boot(sleep_log_t *log);
// Append; the entry's boot field is filled from the log. Full log drops the oldest.
void sleep_log_push(sleep_log_t *log, sleep_event_kind_t kind, uint8_t detail, uint32_t uptime_s,
                    uint32_t arg, int battery_mv);
// Copy entries oldest first; returns how many were copied.
size_t sleep_log_copy(const sleep_log_t *log, sleep_log_entry_t *out, size_t capacity);
const char *sleep_event_name(uint8_t kind);
#ifdef __cplusplus
}
#endif
