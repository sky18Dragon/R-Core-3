#include "read_pico_validation.h"

#include <string.h>

bool read_pico_validation_init(read_pico_validation_session_t *session,
                               const read_pico_validation_ops_t *ops)
{
    if (session == NULL || ops == NULL || ops->clear_white == NULL || ops->confirm_white == NULL ||
        ops->stop == NULL)
        return false;
    *session = (read_pico_validation_session_t) {.state = READ_PICO_VALIDATION_WAIT_CLEAR,
                                                 .ops = *ops,
                                                 .pending_pattern = READ_PICO_PATTERN_COUNT};
    return true;
}

read_pico_validation_command_t read_pico_validation_feed(read_pico_validation_reader_t *reader,
                                                         uint8_t byte)
{
    if (reader == NULL)
        return READ_PICO_VALIDATION_INVALID;
    if (byte != '\r' && byte != '\n') {
        if (byte < 32 || byte > 126 || reader->length >= sizeof(reader->data))
            reader->discard = true;
        if (!reader->discard)
            reader->data[reader->length++] = (char) byte;
        return READ_PICO_VALIDATION_NONE;
    }
    read_pico_validation_command_t command = READ_PICO_VALIDATION_INVALID;
    if (!reader->discard) {
        if (reader->length == 0)
            command = READ_PICO_VALIDATION_NONE;
        else if (reader->length == 11 && memcmp(reader->data, "clear-white", 11) == 0)
            command = READ_PICO_VALIDATION_CLEAR_WHITE;
        else if (reader->length == 13 && memcmp(reader->data, "white-confirm", 13) == 0)
            command = READ_PICO_VALIDATION_CONFIRM_WHITE;
        else if (reader->length == 4 && memcmp(reader->data, "stop", 4) == 0)
            command = READ_PICO_VALIDATION_STOP;
        else if (reader->length == 6 && memcmp(reader->data, "status", 6) == 0)
            command = READ_PICO_VALIDATION_STATUS;
        else {
            static const struct {
                const char *name;
                read_pico_validation_command_t command;
            } patterns[] = {
                {"gc16-white", READ_PICO_VALIDATION_DRAW_WHITE},
                {"gc16-black", READ_PICO_VALIDATION_DRAW_BLACK},
                {"gc16-midgray", READ_PICO_VALIDATION_DRAW_MIDGRAY},
                {"gc16-gray16", READ_PICO_VALIDATION_DRAW_GRAY16},
                {"gc16-parity", READ_PICO_VALIDATION_DRAW_PARITY},
                {"gc16-corners", READ_PICO_VALIDATION_DRAW_CORNERS},
                {"gc16-gray16-labeled", READ_PICO_VALIDATION_DRAW_GRAY16_LABELED},
                {"gc16-gray16-vendor", READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR},
                {"gc16-gray16-page", READ_PICO_VALIDATION_DRAW_GRAY16_PAGE},
                {"pattern-confirm", READ_PICO_VALIDATION_CONFIRM_PATTERN},
                {"pattern-reject", READ_PICO_VALIDATION_REJECT_PATTERN},
            };
            for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i)
                if (reader->length == strlen(patterns[i].name) &&
                    memcmp(reader->data, patterns[i].name, reader->length) == 0) {
                    command = patterns[i].command;
                    break;
                }
        }
    }
    reader->length = 0;
    reader->discard = false;
    return command;
}

bool read_pico_validation_apply(read_pico_validation_session_t *session,
                                read_pico_validation_command_t command)
{
    if (session == NULL || session->ops.clear_white == NULL || session->ops.confirm_white == NULL ||
        session->ops.stop == NULL)
        return false;
    if (command == READ_PICO_VALIDATION_STATUS)
        return true;
    if ((command == READ_PICO_VALIDATION_CONFIRM_PATTERN ||
         command == READ_PICO_VALIDATION_REJECT_PATTERN) &&
        session->state == READ_PICO_VALIDATION_WAIT_PATTERN &&
        (unsigned) session->pending_pattern < READ_PICO_PATTERN_COUNT) {
        if (command == READ_PICO_VALIDATION_REJECT_PATTERN) {
            session->state = READ_PICO_VALIDATION_FAULT;
            return true;
        }
        session->confirmed_mask |= 1u << session->pending_pattern;
        session->pending_pattern = READ_PICO_PATTERN_COUNT;
        session->state = READ_PICO_VALIDATION_REFERENCE_KNOWN;
        return true;
    }
    int err;
    read_pico_validation_state_t next;
    if (command == READ_PICO_VALIDATION_CLEAR_WHITE &&
        session->state == READ_PICO_VALIDATION_WAIT_CLEAR) {
        err = session->ops.clear_white(session->ops.context);
        next = READ_PICO_VALIDATION_WAIT_WHITE;
    } else if (command == READ_PICO_VALIDATION_CONFIRM_WHITE &&
               session->state == READ_PICO_VALIDATION_WAIT_WHITE) {
        err = session->ops.confirm_white(session->ops.context);
        next = READ_PICO_VALIDATION_REFERENCE_KNOWN;
    } else if (command == READ_PICO_VALIDATION_STOP &&
               session->state != READ_PICO_VALIDATION_STOPPED) {
        err = session->ops.stop(session->ops.context);
        next = READ_PICO_VALIDATION_STOPPED;
    } else if (command >= READ_PICO_VALIDATION_DRAW_WHITE &&
               command <= READ_PICO_VALIDATION_DRAW_GRAY16_PAGE &&
               session->state == READ_PICO_VALIDATION_REFERENCE_KNOWN &&
               session->ops.draw_pattern != NULL) {
        read_pico_pattern_t pattern =
            (read_pico_pattern_t) (command - READ_PICO_VALIDATION_DRAW_WHITE);
        uint32_t bit = 1u << pattern;
        // The vendor from-white control may only be the first draw after this boot's
        // manually confirmed white. Never reset a gray back-buffer to pretend it is white.
        if (pattern >= READ_PICO_PATTERN_GRAY16_VENDOR && session->drawn_mask != 0)
            return false;
        if (session->drawn_mask & bit)
            return false;
        err = session->ops.draw_pattern(session->ops.context, pattern);
        if (err == 0) {
            session->drawn_mask |= bit;
            session->pending_pattern = pattern;
        }
        next = READ_PICO_VALIDATION_WAIT_PATTERN;
    } else
        return false;
    session->operation_error = err;
    session->state = err == 0 ? next : READ_PICO_VALIDATION_FAULT;
    if (session->state == READ_PICO_VALIDATION_STOPPED)
        session->pending_pattern = READ_PICO_PATTERN_COUNT;
    return err == 0;
}
