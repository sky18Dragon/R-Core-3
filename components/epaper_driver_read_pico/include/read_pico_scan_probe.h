#pragma once

#include "esp_err.h"
#include "read_pico_scan_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display-validation build only. No IO, allocation, log or retry in callbacks.
// Begin on core 0 while LCD is idle; end after the synchronous clear returns.
esp_err_t read_pico_scan_probe_begin(void);
// Immutable until next begin. No automatic reference confirmation or panel IO.
const read_pico_scan_capture_t *read_pico_scan_probe_end(void);

#define READ_PICO_PHASE_MAX 48
typedef struct {
    int64_t registered_us, start_us, done_us;
    bool started;
} read_pico_phase_t;
typedef struct {
    read_pico_phase_t phases[READ_PICO_PHASE_MAX];
    uint32_t registered, started, completed, stored, errors, dropped;
    bool complete;
} read_pico_phase_capture_t;
// Timestamp only: no line interception. Software start entry is not a pin edge.
esp_err_t read_pico_phase_probe_begin(void);
const read_pico_phase_capture_t *read_pico_phase_probe_end(uint32_t expected);

typedef struct {
    read_pico_edge_capture_t lines;
    read_pico_phase_capture_t phases;
} read_pico_gc16_capture_t;

// gray16-page only: phase timing plus bounded hashes for selected native rows.
// This checks callback order/data relationships, not LCD/DMA output or gate timing.
esp_err_t read_pico_gc16_probe_begin(void);
const read_pico_gc16_capture_t *read_pico_gc16_probe_end(uint32_t expected);

#ifdef __cplusplus
}
#endif
