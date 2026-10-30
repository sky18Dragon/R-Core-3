#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "read_pico_pattern.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    READ_PICO_VALIDATION_NONE = 0,
    READ_PICO_VALIDATION_INVALID,
    READ_PICO_VALIDATION_CLEAR_WHITE,
    READ_PICO_VALIDATION_CONFIRM_WHITE,
    READ_PICO_VALIDATION_STOP,
    READ_PICO_VALIDATION_STATUS,
    READ_PICO_VALIDATION_DRAW_WHITE,
    READ_PICO_VALIDATION_DRAW_BLACK,
    READ_PICO_VALIDATION_DRAW_MIDGRAY,
    READ_PICO_VALIDATION_DRAW_GRAY16,
    READ_PICO_VALIDATION_DRAW_PARITY,
    READ_PICO_VALIDATION_DRAW_CORNERS,
    READ_PICO_VALIDATION_DRAW_GRAY16_LABELED,
    READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR,
    READ_PICO_VALIDATION_DRAW_GRAY16_PAGE,
    READ_PICO_VALIDATION_DRAW_GRAY16_LABELED_CONTINUOUS,
    READ_PICO_VALIDATION_CONFIRM_PATTERN,
    READ_PICO_VALIDATION_REJECT_PATTERN,
} read_pico_validation_command_t;

typedef enum {
    READ_PICO_VALIDATION_WAIT_CLEAR = 0,
    READ_PICO_VALIDATION_WAIT_WHITE,
    READ_PICO_VALIDATION_REFERENCE_KNOWN,
    READ_PICO_VALIDATION_STOPPED,
    READ_PICO_VALIDATION_FAULT,
    READ_PICO_VALIDATION_WAIT_PATTERN,
} read_pico_validation_state_t;

typedef struct {
    void *context;
    int (*clear_white)(void *context);
    int (*confirm_white)(void *context);
    int (*stop)(void *context);
    // Optional: NULL preserves a white-only session; never called automatically.
    int (*draw_pattern)(void *context, read_pico_pattern_t pattern);
    // Optional display-validation control. It must perform white then labeled GC16 as one
    // bounded rail-on transaction and return only after cleanup; no interactive wait while on.
    int (*draw_white_then_labeled)(void *context);
} read_pico_validation_ops_t;

typedef struct {
    read_pico_validation_state_t state;
    int operation_error;
    read_pico_validation_ops_t ops;
    read_pico_pattern_t pending_pattern;
    uint32_t drawn_mask;
    uint32_t confirmed_mask;
} read_pico_validation_session_t;

typedef struct {
    char data[48];
    size_t length;
    bool discard;
} read_pico_validation_reader_t;

// Fresh session only; does not perform any IO or automatically establish a reference.
bool read_pico_validation_init(read_pico_validation_session_t *session,
                               const read_pico_validation_ops_t *ops);
// One bounded ASCII line. Overflow/control bytes discard the WHOLE line until CR/LF.
read_pico_validation_command_t read_pico_validation_feed(read_pico_validation_reader_t *reader,
                                                         uint8_t byte);
// True if a legal command was handled (or its IO succeeded). A handled visual rejection
// intentionally latches FAULT. Reject duplicate draws and wrong stages; only explicit
// stop/status remain available after failure. No automatic clear/retry/draw/confirmation.
bool read_pico_validation_apply(read_pico_validation_session_t *session,
                                read_pico_validation_command_t command);

#ifdef __cplusplus
}
#endif
