#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PHOTO_NONE,
    PHOTO_INVALID,
    PHOTO_STATUS,
    PHOTO_MOUNT,
    PHOTO_CLEAR,
    PHOTO_WHITE_CONFIRM,
    PHOTO_SHOW,
    PHOTO_CONFIRM,
    PHOTO_REJECT,
    PHOTO_STOP
} photo_command_t;
typedef enum {
    PHOTO_WAIT_MOUNT,
    PHOTO_WAIT_CLEAR,
    PHOTO_WAIT_WHITE,
    PHOTO_READY,
    PHOTO_WAIT_VISUAL,
    PHOTO_VERIFIED,
    PHOTO_FAULT,
    PHOTO_STOPPED
} photo_state_t;
typedef struct {
    void *context;
    int (*mount)(void *);
    int (*clear)(void *);
    int (*white_confirm)(void *);
    int (*show)(void *);
    int (*stop)(void *);
} photo_ops_t;
typedef struct {
    photo_ops_t ops;
    photo_state_t state;
    int operation_error, cleanup_error;
    bool stop_attempted, busy;
} photo_session_t;
typedef struct {
    char line[32];
    size_t length;
    bool discard;
} photo_reader_t;

// Zero IO at init; each mount/clear/show/stop attempt is consumed, including failures.
bool photo_session_init(photo_session_t *s, const photo_ops_t *ops);
bool photo_session_apply(photo_session_t *s, photo_command_t command);
photo_command_t photo_session_feed(photo_reader_t *reader, uint8_t byte);
const char *photo_session_state_name(photo_state_t state);

#ifdef __cplusplus
}
#endif
