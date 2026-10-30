#include "read_pico_photo_session.h"

#include <string.h>

bool photo_session_init(photo_session_t *s, const photo_ops_t *ops)
{
    if (!s || !ops || !ops->mount || !ops->clear || !ops->white_confirm || !ops->show || !ops->stop)
        return false;
    photo_ops_t copy = *ops;
    memset(s, 0, sizeof(*s));
    s->ops = copy;
    s->state = PHOTO_WAIT_MOUNT;
    return true;
}

static bool run(photo_session_t *s, int (*operation)(void *), photo_state_t next)
{
    // Consume before IO: no reentrant callback can draw or repeat this operation.
    s->state = PHOTO_FAULT;
    s->busy = true;
    s->operation_error = operation(s->ops.context);
    s->busy = false;
    if (!s->operation_error)
        s->state = next;
    return !s->operation_error;
}

bool photo_session_apply(photo_session_t *s, photo_command_t c)
{
    if (!s || !s->ops.mount || !s->ops.clear || !s->ops.white_confirm || !s->ops.show ||
        !s->ops.stop)
        return false;
    if (c == PHOTO_STATUS)
        return true;
    if (s->busy)
        return false;
    if (c == PHOTO_STOP) {
        if (s->state == PHOTO_STOPPED)
            return true;
        if (s->stop_attempted)
            return false;
        s->stop_attempted = true;
        s->state = PHOTO_FAULT;
        s->busy = true;
        s->cleanup_error = s->ops.stop(s->ops.context);
        s->busy = false;
        s->state = s->cleanup_error ? PHOTO_FAULT : PHOTO_STOPPED;
        return !s->cleanup_error;
    }
    if (c == PHOTO_MOUNT && s->state == PHOTO_WAIT_MOUNT)
        return run(s, s->ops.mount, PHOTO_WAIT_CLEAR);
    if (c == PHOTO_CLEAR && s->state == PHOTO_WAIT_CLEAR)
        return run(s, s->ops.clear, PHOTO_WAIT_WHITE);
    if (c == PHOTO_WHITE_CONFIRM && s->state == PHOTO_WAIT_WHITE)
        return run(s, s->ops.white_confirm, PHOTO_READY);
    if (c == PHOTO_SHOW && s->state == PHOTO_READY)
        return run(s, s->ops.show, PHOTO_WAIT_VISUAL);
    if (c == PHOTO_CONFIRM && s->state == PHOTO_WAIT_VISUAL) {
        s->state = PHOTO_VERIFIED;
        return true;
    }
    if (c == PHOTO_REJECT && (s->state == PHOTO_WAIT_WHITE || s->state == PHOTO_WAIT_VISUAL)) {
        s->operation_error = -1;
        s->state = PHOTO_FAULT;
        return true;
    }
    return false;
}

photo_command_t photo_session_feed(photo_reader_t *r, uint8_t byte)
{
    if (!r)
        return PHOTO_INVALID;
    if (byte == '\r' || byte == '\n') {
        if (!r->length && !r->discard)
            return PHOTO_NONE;
        photo_command_t c = PHOTO_INVALID;
        if (!r->discard) {
            r->line[r->length] = 0;
            const char *names[] = {"status",     "mount",         "clear-white",  "white-confirm",
                                   "photo-show", "photo-confirm", "photo-reject", "stop"};
            const photo_command_t commands[] = {PHOTO_STATUS,        PHOTO_MOUNT, PHOTO_CLEAR,
                                                PHOTO_WHITE_CONFIRM, PHOTO_SHOW,  PHOTO_CONFIRM,
                                                PHOTO_REJECT,        PHOTO_STOP};
            for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
                if (strcmp(r->line, names[i]) == 0)
                    c = commands[i];
        }
        memset(r, 0, sizeof(*r));
        return c;
    }
    if (byte < 0x20 || byte > 0x7e || r->length >= sizeof(r->line) - 1)
        r->discard = true;
    if (!r->discard)
        r->line[r->length++] = byte;
    return PHOTO_NONE;
}

const char *photo_session_state_name(photo_state_t state)
{
    const char *names[] = {"WAIT_MOUNT",  "WAIT_CLEAR", "WAIT_WHITE", "READY",
                           "WAIT_VISUAL", "VERIFIED",   "FAULT",      "STOPPED"};
    return (unsigned) state < sizeof(names) / sizeof(names[0]) ? names[state] : "INVALID";
}
