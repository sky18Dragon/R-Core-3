#pragma once

#include "read_pico_frame.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    READ_PICO_REFRESH_OK = 0,
    READ_PICO_REFRESH_INVALID_ARGUMENT,
    READ_PICO_REFRESH_BUSY,
    READ_PICO_REFRESH_GATE_CLOSED,
    READ_PICO_REFRESH_BAD_FRAME,
    READ_PICO_REFRESH_PREPARE_FAILED,
    READ_PICO_REFRESH_DRAW_FAILED,
    READ_PICO_REFRESH_STOP_FAILED,
    READ_PICO_REFRESH_TEMPERATURE_FAILED,
} read_pico_refresh_error_t;

typedef struct {
    // Produced from a fresh PMU sample and verified board/temperature state by the adapter.
    bool vcom_valid;
    int vcom_mv;
    bool temperature_valid;
    int temperature_c;
} read_pico_refresh_gate_t;

typedef struct {
    void *context;
    // Nonblocking exclusive ownership, shared by ALL display/power operations.
    bool (*try_acquire)(void *context);
    void (*release)(void *context);
    // Zero means success. prepare must verify rails/PGOOD; no calibration writes.
    // stop must be safe after a partially failed prepare; it must report cleanup failures.
    int (*prepare)(void *context, int vcom_mv);
    int (*draw_gc16)(void *context, const uint8_t *frame, size_t size, int temperature_c);
    int (*stop)(void *context);
    // Optional active-only sensor. When present, gate.temperature_* are ignored;
    // sample after successful prepare under the same lock, before drawing. A sensor
    // error or out-of-range value forbids draw and still invokes stop. Never supply
    // a default temperature. NULL retains the prevalidated-temperature contract.
    int (*read_temperature)(void *context, int *temperature_c);
} read_pico_refresh_ops_t;

typedef struct {
    read_pico_refresh_error_t error;
    read_pico_frame_error_t frame_error;
    int operation_error;
    int cleanup_error;
    // Number of GC16 frames whose draw callback returned success before cleanup.
    // One for read_pico_refresh_gc16; zero to two for the continuous pair API.
    unsigned frames_completed;
    // True only if GC16 AND cleanup succeeded. Never substitute this for real-screen QA.
    bool completed;
} read_pico_refresh_result_t;

// Synchronous transaction for ONE 1216x684 GC16 frame. Borrowed buffers remain valid
// until return; callbacks must not retain them or release the lock early.
// This core supplies no hardware transport; P1 never invokes it with real board I/O.
read_pico_refresh_result_t read_pico_refresh_gc16(const read_pico_refresh_ops_t *ops,
                                                  read_pico_refresh_gate_t gate,
                                                  const uint8_t *source, size_t source_size,
                                                  uint8_t *scratch, size_t scratch_size);

// Synchronous two-frame transaction under one lock, one prepare/active-temperature sample,
// and one stop. Both inputs are fully validated before prepare. The second draw starts only
// after the first succeeds; any failure still invokes stop before releasing ownership.
read_pico_refresh_result_t read_pico_refresh_gc16_pair(
    const read_pico_refresh_ops_t *ops, read_pico_refresh_gate_t gate, const uint8_t *first_source,
    size_t first_size, const uint8_t *second_source, size_t second_size, uint8_t *scratch,
    size_t scratch_size);

#ifdef __cplusplus
}
#endif
