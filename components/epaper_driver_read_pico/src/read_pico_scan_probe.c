#include "read_pico_scan_probe.h"

#include <string.h>

#include "esp_attr.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "output_lcd/lcd_driver.h"

// Link wrappers preserve the vendor's callbacks/payloads/return values. Instrument
// static-clear samples, phase timestamps, or the explicit gray16-page edge probe.
// Software observations add overhead and are not DMA data or panel pin edges.
static DRAM_ATTR portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static DRAM_ATTR read_pico_scan_capture_t capture;
static DRAM_ATTR read_pico_scan_capture_t frozen;
static DRAM_ATTR read_pico_scan_frame_t current;
static DRAM_ATTR bool active, frame_open;
static DRAM_ATTR uint32_t generation, in_flight;
static DRAM_ATTR line_cb_func_t line_fn;
static DRAM_ATTR frame_done_func_t frame_fn;
static DRAM_ATTR void *line_payload, *frame_payload;
static DRAM_ATTR bool phase_active, phase_open;
static DRAM_ATTR read_pico_phase_capture_t phases, phase_frozen;
static DRAM_ATTR read_pico_phase_t phase_current;
static DRAM_ATTR bool edge_active, edge_frame_open;
static DRAM_ATTR uint32_t edge_generation, edge_in_flight;
static DRAM_ATTR read_pico_edge_capture_t edge_capture;
static DRAM_ATTR read_pico_edge_frame_t edge_current;
static DRAM_ATTR read_pico_gc16_capture_t gc16_frozen;

void __real_epd_lcd_line_source_cb(line_cb_func_t fn, void *payload);
void __real_epd_lcd_frame_done_cb(frame_done_func_t fn, void *payload);
void __real_epd_lcd_start_frame(void);

static int IRAM_ATTR edge_slot(uint32_t row)
{
    switch (row) {
    case 683:
        return 0;
    case 644:
        return 1;
    case 643:
        return 2;
    case 607:
        return 3;
    case 606:
        return 4;
    case 604:
        return 5;
    case 603:
        return 6;
    case 570:
        return 7;
    case 569:
        return 8;
    case 564:
        return 9;
    case 563:
        return 10;
    case 556:
        return 11;
    default:
        return -1;
    }
}

static uint32_t IRAM_ATTR line_hash(const uint8_t *bytes)
{
    uint32_t hash = 2166136261u;
    for (unsigned i = 0; i < 304; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

void IRAM_ATTR __wrap_epd_lcd_start_frame(void)
{
    portENTER_CRITICAL_SAFE(&mux);
    if (phase_active) {
        ++phases.started;
        if (!phase_open || phase_current.started)
            ++phases.errors;
        else {
            phase_current.start_us = esp_timer_get_time();
            phase_current.started = true;
        }
    }
    portEXIT_CRITICAL_SAFE(&mux);
    __real_epd_lcd_start_frame();
}

static bool IRAM_ATTR traced_line(void *unused, uint8_t *bytes)
{
    (void) unused;
    portENTER_CRITICAL_SAFE(&mux);
    line_cb_func_t fn = line_fn;
    void *payload = line_payload;
    bool recording = active && frame_open;
    uint32_t row = current.source_calls, token = generation;
    bool edge_recording = edge_active && edge_frame_open;
    uint32_t edge_row = edge_current.source_calls, edge_token = edge_generation;
    if (recording) {
        ++current.source_calls;
        ++in_flight;
    } else if (active) {
        ++capture.orphan_lines;
    }
    if (edge_recording) {
        ++edge_current.source_calls;
        ++edge_in_flight;
    } else if (edge_active) {
        ++edge_capture.orphan_lines;
    }
    if (active && xPortGetCoreID() != 0)
        ++capture.wrong_core;
    if (edge_active && xPortGetCoreID() != 0) {
        if (edge_recording && edge_row < READ_PICO_EDGE_START_PREFILL_CALLS)
            ++edge_capture.core1_start_prefill;
        else
            ++edge_capture.wrong_core;
    }
    portEXIT_CRITICAL_SAFE(&mux);
    bool result = fn ? fn(payload, bytes) : false;
    int slot = edge_recording ? edge_slot(edge_row) : -1;
    uint32_t hash = slot >= 0 && bytes != NULL ? line_hash(bytes) : 0;
    portENTER_CRITICAL_SAFE(&mux);
    if (recording) {
        --in_flight;
        if (!frame_open || generation != token) {
            ++capture.late_lines;
        } else if (row >= READ_PICO_TRACE_FIRST_ROW &&
                   row < READ_PICO_TRACE_FIRST_ROW + READ_PICO_TRACE_ROWS && bytes != NULL) {
            unsigned slot = row - READ_PICO_TRACE_FIRST_ROW;
            current.first[slot] = bytes[0];
            current.last[slot] = bytes[303];  // 1216 two-bit pulse pixels / 4.
            current.seen_rows |= 1u << slot;
        }
    }
    if (edge_recording) {
        --edge_in_flight;
        if (!edge_frame_open || edge_generation != edge_token) {
            ++edge_capture.late_lines;
        } else if (slot >= 0 && bytes != NULL) {
            edge_current.hashes[slot] = hash;
            edge_current.seen_rows |= 1u << slot;
        }
    }
    portEXIT_CRITICAL_SAFE(&mux);
    return result;
}

static void IRAM_ATTR traced_done(void *unused)
{
    (void) unused;
    portENTER_CRITICAL_SAFE(&mux);
    frame_done_func_t fn = frame_fn;
    void *payload = frame_payload;
    if (phase_active) {
        ++phases.completed;
        if (!phase_open || !phase_current.started)
            ++phases.errors;
        if (phases.stored < READ_PICO_PHASE_MAX) {
            read_pico_phase_t *out = &phases.phases[phases.stored++];
            out->registered_us = phase_current.registered_us;
            out->start_us = phase_current.start_us;
            out->done_us = esp_timer_get_time();
            out->started = phase_current.started;
            if (out->registered_us > out->start_us || out->start_us > out->done_us)
                ++phases.errors;
        } else
            ++phases.dropped;
        phase_open = false;
    }
    if (active) {
        if (xPortGetCoreID() != 0)
            ++capture.wrong_core;
        ++capture.completed_frames;
        if (!frame_open || in_flight)
            ++capture.unfinished_frames;
        if (capture.stored_frames < READ_PICO_TRACE_MAX_FRAMES) {
            // Scalar/byte stores, no flash-resident memcpy from this ISR.
            read_pico_scan_frame_t *out = &capture.frames[capture.stored_frames++];
            out->source_calls = current.source_calls;
            out->seen_rows = current.seen_rows;
            for (unsigned i = 0; i < READ_PICO_TRACE_ROWS; ++i) {
                out->first[i] = current.first[i];
                out->last[i] = current.last[i];
            }
        } else {
            ++capture.dropped_frames;
        }
        frame_open = false;
    }
    if (edge_active) {
        if (xPortGetCoreID() != 0)
            ++edge_capture.wrong_core;
        ++edge_capture.completed_frames;
        if (!edge_frame_open || edge_in_flight)
            ++edge_capture.unfinished_frames;
        if (edge_capture.stored_frames < READ_PICO_EDGE_TRACE_MAX_FRAMES) {
            read_pico_edge_frame_t *out = &edge_capture.frames[edge_capture.stored_frames++];
            out->source_calls = edge_current.source_calls;
            out->seen_rows = edge_current.seen_rows;
            for (unsigned i = 0; i < READ_PICO_EDGE_TRACE_ROWS; ++i)
                out->hashes[i] = edge_current.hashes[i];
        } else {
            ++edge_capture.dropped_frames;
        }
        edge_frame_open = false;
    }
    portEXIT_CRITICAL_SAFE(&mux);
    // The original frame callback can unregister itself; use saved locals.
    if (fn)
        fn(payload);
}

void IRAM_ATTR __wrap_epd_lcd_line_source_cb(line_cb_func_t fn, void *payload)
{
    portENTER_CRITICAL_SAFE(&mux);
    line_fn = fn;
    line_payload = payload;
    bool instrument = (active || edge_active) && fn != NULL;
    portEXIT_CRITICAL_SAFE(&mux);
    __real_epd_lcd_line_source_cb(instrument ? traced_line : fn, instrument ? NULL : payload);
}

void IRAM_ATTR __wrap_epd_lcd_frame_done_cb(frame_done_func_t fn, void *payload)
{
    portENTER_CRITICAL_SAFE(&mux);
    frame_fn = fn;
    frame_payload = payload;
    bool instrument = (active || edge_active || phase_active) && fn != NULL;
    if (phase_active && fn != NULL) {
        if (phase_open)
            ++phases.errors;
        ++phases.registered;
        phase_current.registered_us = esp_timer_get_time();
        phase_current.start_us = phase_current.done_us = 0;
        phase_current.started = false;
        phase_open = true;
    }
    if (active && fn != NULL) {
        if (frame_open || in_flight)
            ++capture.unfinished_frames;
        ++capture.registered_frames;
        ++generation;
        current.source_calls = 0;
        current.seen_rows = 0;
        for (unsigned i = 0; i < READ_PICO_TRACE_ROWS; ++i)
            current.first[i] = current.last[i] = 0;
        frame_open = true;
    }
    if (edge_active && fn != NULL) {
        if (edge_frame_open || edge_in_flight)
            ++edge_capture.unfinished_frames;
        ++edge_capture.registered_frames;
        ++edge_generation;
        edge_current.source_calls = 0;
        edge_current.seen_rows = 0;
        for (unsigned i = 0; i < READ_PICO_EDGE_TRACE_ROWS; ++i)
            edge_current.hashes[i] = 0;
        edge_frame_open = true;
    }
    portEXIT_CRITICAL_SAFE(&mux);
    __real_epd_lcd_frame_done_cb(instrument ? traced_done : fn, instrument ? NULL : payload);
}

esp_err_t read_pico_scan_probe_begin(void)
{
    if (xPortGetCoreID() != 0)
        return ESP_ERR_INVALID_STATE;
    portENTER_CRITICAL_SAFE(&mux);
    bool busy = active || edge_active || phase_active || frame_open || edge_frame_open ||
                in_flight || edge_in_flight || line_fn || frame_fn;
    portEXIT_CRITICAL_SAFE(&mux);
    if (busy)
        return ESP_ERR_INVALID_STATE;
    // Idle core-0 caller owns this static clear probe. No reset inside the ISR.
    memset(&capture, 0, sizeof(capture));
    memset(&current, 0, sizeof(current));
    portENTER_CRITICAL_SAFE(&mux);
    active = true;
    portEXIT_CRITICAL_SAFE(&mux);
    return ESP_OK;
}

const read_pico_scan_capture_t *read_pico_scan_probe_end(void)
{
    portENTER_CRITICAL_SAFE(&mux);
    if (active && (frame_open || in_flight))
        ++capture.unfinished_frames;
    active = false;
    frame_open = false;
    frozen = capture;  // Task-only snapshot; later callbacks cannot mutate the returned data.
    line_cb_func_t source = line_fn;
    frame_done_func_t done = frame_fn;
    void *source_arg = line_payload, *done_arg = frame_payload;
    portEXIT_CRITICAL_SAFE(&mux);
    // Restore the exact last registrations; never leave an observer installed.
    __real_epd_lcd_line_source_cb(source, source_arg);
    __real_epd_lcd_frame_done_cb(done, done_arg);
    return &frozen;
}

esp_err_t read_pico_phase_probe_begin(void)
{
    if (xPortGetCoreID() != 0)
        return ESP_ERR_INVALID_STATE;
    portENTER_CRITICAL_SAFE(&mux);
    bool busy = active || edge_active || phase_active || frame_open || edge_frame_open ||
                in_flight || edge_in_flight || line_fn || frame_fn;
    if (!busy) {
        memset(&phases, 0, sizeof(phases));
        memset(&phase_current, 0, sizeof(phase_current));
        phase_open = false;
        phase_active = true;
    }
    portEXIT_CRITICAL_SAFE(&mux);
    return busy ? ESP_ERR_INVALID_STATE : ESP_OK;
}

const read_pico_phase_capture_t *read_pico_phase_probe_end(uint32_t expected)
{
    portENTER_CRITICAL_SAFE(&mux);
    if (phase_open)
        ++phases.errors;
    phases.complete = phase_active && expected > 0 && expected <= READ_PICO_PHASE_MAX &&
                      phases.registered == expected && phases.started == expected &&
                      phases.completed == expected && phases.stored == expected && !phases.errors &&
                      !phases.dropped;
    phase_active = phase_open = false;
    phase_frozen = phases;
    frame_done_func_t done = frame_fn;
    void *arg = frame_payload;
    portEXIT_CRITICAL_SAFE(&mux);
    __real_epd_lcd_frame_done_cb(done, arg);
    return &phase_frozen;
}

esp_err_t read_pico_gc16_probe_begin(void)
{
    if (xPortGetCoreID() != 0)
        return ESP_ERR_INVALID_STATE;
    portENTER_CRITICAL_SAFE(&mux);
    bool busy = active || edge_active || phase_active || frame_open || edge_frame_open ||
                in_flight || edge_in_flight || line_fn || frame_fn;
    if (!busy) {
        memset(&edge_capture, 0, sizeof(edge_capture));
        memset(&edge_current, 0, sizeof(edge_current));
        memset(&phases, 0, sizeof(phases));
        memset(&phase_current, 0, sizeof(phase_current));
        edge_frame_open = phase_open = false;
        edge_active = phase_active = true;
    }
    portEXIT_CRITICAL_SAFE(&mux);
    return busy ? ESP_ERR_INVALID_STATE : ESP_OK;
}

const read_pico_gc16_capture_t *read_pico_gc16_probe_end(uint32_t expected)
{
    portENTER_CRITICAL_SAFE(&mux);
    if (edge_active && (edge_frame_open || edge_in_flight))
        ++edge_capture.unfinished_frames;
    if (phase_open)
        ++phases.errors;
    phases.complete = phase_active && expected > 0 && expected <= READ_PICO_PHASE_MAX &&
                      phases.registered == expected && phases.started == expected &&
                      phases.completed == expected && phases.stored == expected && !phases.errors &&
                      !phases.dropped;
    edge_active = edge_frame_open = phase_active = phase_open = false;
    gc16_frozen.lines = edge_capture;
    gc16_frozen.phases = phases;
    line_cb_func_t source = line_fn;
    frame_done_func_t done = frame_fn;
    void *source_arg = line_payload, *done_arg = frame_payload;
    portEXIT_CRITICAL_SAFE(&mux);
    __real_epd_lcd_line_source_cb(source, source_arg);
    __real_epd_lcd_frame_done_cb(done, done_arg);
    return &gc16_frozen;
}
