#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    READ_PICO_TRACE_ROWS = 8,  // Logical source callback rows 680..687.
    READ_PICO_TRACE_FIRST_ROW = 680,
    READ_PICO_TRACE_MAX_FRAMES = 96,
    READ_PICO_CLEAR_EXPECTED_FRAMES = 78,  // Locked vendor: 3 * (10 + 13 + 3).
    READ_PICO_EDGE_TRACE_ROWS = 12,
    READ_PICO_EDGE_TRACE_MAX_FRAMES = 48,
    READ_PICO_EDGE_EXPECTED_FRAMES = 36,
    // Both geometries observed so far produce 696 software callbacks: the pinned
    // driver emits 688 rows plus two four-line refills; the exact-line candidate
    // emits 684 rows plus two six-line refills. This is not a physical line count.
    READ_PICO_EDGE_EXPECTED_SOURCE_CALLS = 696,
    READ_PICO_EDGE_START_PREFILL_CALLS = 12,
};

typedef struct {
    uint32_t source_calls;
    uint8_t seen_rows;
    uint8_t first[READ_PICO_TRACE_ROWS];
    uint8_t last[READ_PICO_TRACE_ROWS];
} read_pico_scan_frame_t;

typedef struct {
    uint32_t registered_frames, completed_frames, stored_frames;
    uint32_t dropped_frames, orphan_lines, late_lines, unfinished_frames, wrong_core;
    read_pico_scan_frame_t frames[READ_PICO_TRACE_MAX_FRAMES];
} read_pico_scan_capture_t;

typedef struct {
    bool software_complete;
    uint32_t min_source_calls, max_source_calls;
    uint32_t missing_tail_frames, missing_padding_frames, nonzero_padding_frames;
    uint32_t unequal_edge_frames, unexpected_pulse_frames;
} read_pico_scan_summary_t;

// Pure analysis of software callbacks. Not proof of DMA transmission, CKV/STV,
// analog rails or visible pixels. Tail samples are pulse bytes, NOT 4bpp pixels.
bool read_pico_scan_analyze(const read_pico_scan_capture_t *capture,
                            read_pico_scan_summary_t *summary);

// Native landscape rows for logical portrait u={0,39,40,76,77,79,80,113,
// 114,119,120,127}. Adjacent pairs have identical expected page content;
// each pair-to-pair boundary must differ during at least one GC16 phase.
typedef struct {
    uint32_t source_calls;
    uint16_t seen_rows;
    uint32_t hashes[READ_PICO_EDGE_TRACE_ROWS];
} read_pico_edge_frame_t;

typedef struct {
    uint32_t registered_frames, completed_frames, stored_frames;
    uint32_t dropped_frames, orphan_lines, late_lines, unfinished_frames;
    // The display-validation overlay synchronously prefills rows 0..11 on CPU1
    // while epd_lcd_start_frame fills its two six-line buffers. Later CPU1
    // callbacks are faults.
    uint32_t core1_start_prefill, wrong_core;
    read_pico_edge_frame_t frames[READ_PICO_EDGE_TRACE_MAX_FRAMES];
} read_pico_edge_capture_t;

typedef struct {
    bool software_complete;
    uint32_t min_source_calls, max_source_calls;
    uint32_t missing_rows_frames, unequal_pair_frames;
    uint8_t indistinguishable_boundaries;
    uint32_t trajectory_hashes[READ_PICO_EDGE_TRACE_ROWS];
} read_pico_edge_summary_t;

uint16_t read_pico_edge_trace_row(unsigned slot);
bool read_pico_edge_analyze(const read_pico_edge_capture_t *capture,
                            read_pico_edge_summary_t *summary);

#ifdef __cplusplus
}
#endif
