#include "sd_file_probe.h"

uint8_t sd_file_probe_byte(size_t offset)
{
    return (uint8_t) (offset * 37u + (offset >> 8) * 17u + 0x5au);
}

static bool ready(const sd_file_probe_read_io_t *io, sd_file_probe_result_t *result)
{
    if (io->media_ready(io->context))
        return true;
    result->error = SD_FILE_PROBE_MEDIA_UNAVAILABLE;
    return false;
}

static bool close_file(sd_file_probe_session_t *session, const sd_file_probe_read_io_t *io,
                       int fd, sd_file_probe_result_t *result)
{
    // Close even after media failure; do not re-use or retry an ambiguous handle.
    if (io->close(io->context, fd) == 0)
        return true;
    session->close_failed = true;
    if (result->error == SD_FILE_PROBE_OK)
        result->error = SD_FILE_PROBE_CLOSE_FAILED;
    return false;
}

static void check_contents(sd_file_probe_session_t *session, const sd_file_probe_read_io_t *io,
                           sd_file_probe_result_t *result, sd_file_probe_error_t open_error)
{
    if (!ready(io, result))
        return;
    int fd = io->open_read(io->context);
    if (fd < 0) {
        result->error = open_error;
        return;
    }
    uint8_t buffer[SD_FILE_PROBE_CHUNK];
    while (result->verified < SD_FILE_PROBE_SIZE && ready(io, result)) {
        size_t size = SD_FILE_PROBE_SIZE - result->verified;
        if (size > sizeof(buffer))
            size = sizeof(buffer);
        int count = io->read(io->context, fd, buffer, size);
        if (count <= 0 || (size_t) count > size) {
            result->error = SD_FILE_PROBE_READ_FAILED;
            break;
        }
        for (int i = 0; i < count; ++i) {
            if (buffer[i] != sd_file_probe_byte(result->verified)) {
                result->error = SD_FILE_PROBE_MISMATCH;
                break;
            }
            ++result->verified;
        }
        if (result->error != SD_FILE_PROBE_OK)
            break;
    }
    if (result->error == SD_FILE_PROBE_OK && ready(io, result)) {
        int count = io->read(io->context, fd, buffer, 1);
        if (count < 0)
            result->error = SD_FILE_PROBE_READ_FAILED;
        else if (count != 0)
            result->error = SD_FILE_PROBE_EXTRA_DATA;
    }
    bool closed = close_file(session, io, fd, result);
    if (closed && result->error == SD_FILE_PROBE_OK)
        (void) ready(io, result);
}

sd_file_probe_result_t sd_file_probe_run(sd_file_probe_session_t *session,
                                       const sd_file_probe_io_t *io)
{
    sd_file_probe_result_t result = {.error = SD_FILE_PROBE_INVALID};
    if (session == NULL || io == NULL || io->media_ready == NULL || io->open_new == NULL ||
        io->open_read == NULL || io->write == NULL || io->read == NULL ||
        io->sync == NULL || io->close == NULL)
        return result;
    if (session->attempted || session->close_failed) {
        result.error = SD_FILE_PROBE_ALREADY_ATTEMPTED;
        return result;
    }
    const sd_file_probe_read_io_t read_io = {
        .context = io->context, .media_ready = io->media_ready,
        .open_read = io->open_read, .read = io->read, .close = io->close,
    };
    session->attempted = true;
    result.error = SD_FILE_PROBE_OK;
    if (!ready(&read_io, &result))
        return result;
    int fd = io->open_new(io->context);
    if (fd < 0) {
        result.error = SD_FILE_PROBE_CREATE_FAILED;
        return result;
    }
    result.created = true;
    uint8_t buffer[SD_FILE_PROBE_CHUNK];
    while (result.written < SD_FILE_PROBE_SIZE && ready(&read_io, &result)) {
        size_t size = SD_FILE_PROBE_SIZE - result.written;
        if (size > sizeof(buffer))
            size = sizeof(buffer);
        for (size_t i = 0; i < size; ++i)
            buffer[i] = sd_file_probe_byte(result.written + i);
        int count = io->write(io->context, fd, buffer, size);
        if (count <= 0 || (size_t) count > size) {
            result.error = SD_FILE_PROBE_WRITE_FAILED;
            break;
        }
        result.written += (size_t) count;
    }
    if (result.error == SD_FILE_PROBE_OK && ready(&read_io, &result) &&
        io->sync(io->context, fd) != 0)
        result.error = SD_FILE_PROBE_SYNC_FAILED;
    if (!close_file(session, &read_io, fd, &result) || result.error != SD_FILE_PROBE_OK)
        return result;
    check_contents(session, &read_io, &result, SD_FILE_PROBE_REOPEN_FAILED);
    return result;
}

sd_file_probe_result_t sd_file_probe_check(sd_file_probe_session_t *session,
                                         const sd_file_probe_read_io_t *io)
{
    sd_file_probe_result_t result = {.error = SD_FILE_PROBE_INVALID};
    if (session == NULL || io == NULL || io->media_ready == NULL ||
        io->open_read == NULL || io->read == NULL || io->close == NULL)
        return result;
    if (session->check_attempted || session->close_failed) {
        result.error = SD_FILE_PROBE_ALREADY_ATTEMPTED;
        return result;
    }
    session->check_attempted = true;
    result.error = SD_FILE_PROBE_OK;
    check_contents(session, io, &result, SD_FILE_PROBE_READ_OPEN_FAILED);
    return result;
}

const char *sd_file_probe_error_name(sd_file_probe_error_t error)
{
    switch (error) {
    case SD_FILE_PROBE_OK: return "OK";
    case SD_FILE_PROBE_INVALID: return "INVALID";
    case SD_FILE_PROBE_ALREADY_ATTEMPTED: return "ALREADY_ATTEMPTED";
    case SD_FILE_PROBE_MEDIA_UNAVAILABLE: return "MEDIA_UNAVAILABLE";
    case SD_FILE_PROBE_CREATE_FAILED: return "CREATE_FAILED";
    case SD_FILE_PROBE_WRITE_FAILED: return "WRITE_FAILED";
    case SD_FILE_PROBE_SYNC_FAILED: return "SYNC_FAILED";
    case SD_FILE_PROBE_CLOSE_FAILED: return "CLOSE_FAILED";
    case SD_FILE_PROBE_REOPEN_FAILED: return "REOPEN_FAILED";
    case SD_FILE_PROBE_READ_FAILED: return "READ_FAILED";
    case SD_FILE_PROBE_MISMATCH: return "MISMATCH";
    case SD_FILE_PROBE_EXTRA_DATA: return "EXTRA_DATA";
    case SD_FILE_PROBE_READ_OPEN_FAILED: return "READ_OPEN_FAILED";
    }
    return "INVALID";
}
