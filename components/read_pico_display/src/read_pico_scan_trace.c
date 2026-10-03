#include "read_pico_scan_trace.h"

#include <stddef.h>

bool read_pico_scan_analyze(const read_pico_scan_capture_t *c, read_pico_scan_summary_t *out)
{
    if (c == NULL || out == NULL || c->stored_frames > READ_PICO_TRACE_MAX_FRAMES)
        return false;
    read_pico_scan_summary_t result = {0};
    result.min_source_calls = c->stored_frames ? UINT32_MAX : 0;
    for (uint32_t i = 0; i < c->stored_frames; ++i) {
        const read_pico_scan_frame_t *f = &c->frames[i];
        if (f->source_calls < result.min_source_calls)
            result.min_source_calls = f->source_calls;
        if (f->source_calls > result.max_source_calls)
            result.max_source_calls = f->source_calls;
        if (f->source_calls < 684 || (f->seen_rows & 0x0f) != 0x0f)
            ++result.missing_tail_frames;
        if (f->source_calls < 688 || (f->seen_rows & 0xf0) != 0xf0)
            ++result.missing_padding_frames;
        // Locked vendor clear: 10 dark, 13 white, 3 no-op frames per cycle.
        unsigned phase = i % 26;
        uint8_t expected = phase < 10 ? 0x55 : phase < 23 ? 0xaa : 0x00;
        bool padding = false, unequal = false, unexpected = false;
        for (unsigned row = 0; row < READ_PICO_TRACE_ROWS; ++row) {
            if (!(f->seen_rows & (1u << row)))
                continue;
            if (row >= 4 && (f->first[row] || f->last[row]))
                padding = true;
            if (f->first[row] != f->last[row])
                unequal = true;
            if (row < 4 && (f->first[row] != expected || f->last[row] != expected))
                unexpected = true;
        }
        result.nonzero_padding_frames += padding;
        result.unequal_edge_frames += unequal;
        result.unexpected_pulse_frames += unexpected;
    }
    result.software_complete = c->registered_frames == READ_PICO_CLEAR_EXPECTED_FRAMES &&
                               c->completed_frames == READ_PICO_CLEAR_EXPECTED_FRAMES &&
                               c->stored_frames == READ_PICO_CLEAR_EXPECTED_FRAMES &&
                               !c->dropped_frames && !c->orphan_lines && !c->late_lines &&
                               !c->unfinished_frames && !c->wrong_core &&
                               !result.missing_tail_frames && !result.missing_padding_frames &&
                               !result.nonzero_padding_frames && !result.unequal_edge_frames &&
                               !result.unexpected_pulse_frames;
    *out = result;
    return true;
}

uint16_t read_pico_edge_trace_row(unsigned slot)
{
    static const uint16_t rows[READ_PICO_EDGE_TRACE_ROWS] = {
        683, 644, 643, 607, 606, 604, 603, 570, 569, 564, 563, 556,
    };
    return slot < READ_PICO_EDGE_TRACE_ROWS ? rows[slot] : UINT16_MAX;
}

static uint32_t trajectory_add(uint32_t hash, uint32_t value)
{
    for (unsigned shift = 0; shift < 32; shift += 8) {
        hash ^= (value >> shift) & 0xffu;
        hash *= 16777619u;
    }
    return hash;
}

bool read_pico_edge_analyze(const read_pico_edge_capture_t *c, read_pico_edge_summary_t *out)
{
    if (c == NULL || out == NULL || c->stored_frames > READ_PICO_EDGE_TRACE_MAX_FRAMES)
        return false;
    read_pico_edge_summary_t result = {0};
    result.min_source_calls = c->stored_frames ? UINT32_MAX : 0;
    for (unsigned slot = 0; slot < READ_PICO_EDGE_TRACE_ROWS; ++slot)
        result.trajectory_hashes[slot] = 2166136261u;
    uint8_t distinguished = 0;
    for (uint32_t frame = 0; frame < c->stored_frames; ++frame) {
        const read_pico_edge_frame_t *f = &c->frames[frame];
        if (f->source_calls < result.min_source_calls)
            result.min_source_calls = f->source_calls;
        if (f->source_calls > result.max_source_calls)
            result.max_source_calls = f->source_calls;
        if (f->seen_rows != (1u << READ_PICO_EDGE_TRACE_ROWS) - 1u)
            ++result.missing_rows_frames;
        bool unequal_pair = false;
        for (unsigned pair = 0; pair < READ_PICO_EDGE_TRACE_ROWS / 2; ++pair) {
            unsigned first = pair * 2;
            if (f->hashes[first] != f->hashes[first + 1])
                unequal_pair = true;
            if (pair + 1 < READ_PICO_EDGE_TRACE_ROWS / 2 &&
                f->hashes[first] != f->hashes[first + 2])
                distinguished |= 1u << pair;
        }
        result.unequal_pair_frames += unequal_pair;
        for (unsigned slot = 0; slot < READ_PICO_EDGE_TRACE_ROWS; ++slot)
            result.trajectory_hashes[slot] =
                trajectory_add(result.trajectory_hashes[slot], f->hashes[slot]);
    }
    result.indistinguishable_boundaries =
        (uint8_t) (~distinguished) & ((1u << (READ_PICO_EDGE_TRACE_ROWS / 2 - 1)) - 1u);
    result.software_complete = c->registered_frames == READ_PICO_EDGE_EXPECTED_FRAMES &&
                               c->completed_frames == READ_PICO_EDGE_EXPECTED_FRAMES &&
                               c->stored_frames == READ_PICO_EDGE_EXPECTED_FRAMES &&
                               !c->dropped_frames && !c->orphan_lines && !c->late_lines &&
                               !c->unfinished_frames && !c->wrong_core &&
                               result.min_source_calls == READ_PICO_EDGE_EXPECTED_SOURCE_CALLS &&
                               result.max_source_calls == READ_PICO_EDGE_EXPECTED_SOURCE_CALLS &&
                               !result.missing_rows_frames && !result.unequal_pair_frames &&
                               !result.indistinguishable_boundaries;
    *out = result;
    return true;
}
