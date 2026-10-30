#include "jpeg_file_probe.h"

typedef struct {
    const jpeg_file_probe_io_t *io;
    int fd;
    size_t remaining;
} source_t;

static bool ready(void *context)
{
    source_t *source = context;
    return source->io->media_ready(source->io->context);
}
static int read_input(void *context, uint8_t *data, size_t size)
{
    source_t *source = context;
    if (size > source->remaining)
        size = source->remaining;
    if (!size)
        return 0;
    int got = source->io->read(source->io->context, source->fd, data, size);
    if (got > 0 && (size_t) got > size)
        return -1;
    if (got > 0 && (size_t) got <= size)
        source->remaining -= (size_t) got;
    return got;
}

jpeg_file_probe_result_t jpeg_file_probe_run(jpeg_file_probe_session_t *session,
                                            const jpeg_file_probe_io_t *io,
                                            void *work, size_t work_size)
{
    jpeg_file_probe_result_t result = {.error = JPEG_FILE_INVALID,
                                      .stream = {.error = JPEG_STREAM_INVALID}};
    if (!session || !io || !io->media_ready || !io->open_read || !io->file_size ||
        !io->read || !io->close)
        return result;
    if (session->attempted || session->close_failed) {
        result.error = JPEG_FILE_ALREADY_ATTEMPTED;
        return result;
    }
    session->attempted = true;
    if (!work || work_size < 3100)
        return result;
    if (!io->media_ready(io->context)) {
        result.error = JPEG_FILE_MEDIA_FAILED;
        return result;
    }
    int fd = io->open_read(io->context);
    if (fd < 0) {
        result.error = JPEG_FILE_OPEN_FAILED;
        return result;
    }
    result.error = JPEG_FILE_MEDIA_FAILED;
    if (io->media_ready(io->context)) {
        int64_t size = io->file_size(io->context, fd);
        result.error = JPEG_FILE_SIZE_FAILED;
        if (size > 0 && size <= JPEG_FILE_PROBE_MAX_SIZE) {
            result.file_size = (size_t) size;
            source_t source = {.io = io, .fd = fd, .remaining = result.file_size};
            const jpeg_stream_probe_io_t stream_io = {&source, ready, read_input};
            result.stream = jpeg_stream_probe_run(&stream_io, work, work_size, 2);
            result.error = result.stream.error == JPEG_STREAM_OK ? JPEG_FILE_OK : JPEG_FILE_STREAM_FAILED;
            // A JPEG can contain trailing metadata/another codestream. Check all source
            // bytes and EOF even if the decoder finishes early; do not call that tail decoded.
            uint8_t tail[256];
            while (result.error == JPEG_FILE_OK && source.remaining) {
                if (!ready(&source)) {
                    result.error = JPEG_FILE_MEDIA_FAILED;
                    break;
                }
                int got = read_input(&source, tail, sizeof(tail));
                if (got <= 0)
                    result.error = JPEG_FILE_READ_FAILED;
            }
            result.bytes_read = result.file_size - source.remaining;
            if (result.error == JPEG_FILE_OK) {
                if (!ready(&source))
                    result.error = JPEG_FILE_MEDIA_FAILED;
                else if (io->read(io->context, fd, tail, 1) != 0)
                    result.error = JPEG_FILE_READ_FAILED;
            }
        }
    }
    if (io->close(io->context, fd) != 0) {
        session->close_failed = true;
        if (result.error == JPEG_FILE_OK)
            result.error = JPEG_FILE_CLOSE_FAILED;
    }
    if (result.error == JPEG_FILE_OK && !io->media_ready(io->context))
        result.error = JPEG_FILE_MEDIA_FAILED;
    return result;
}

const char *jpeg_file_probe_error_name(jpeg_file_probe_error_t error)
{
    switch (error) {
    case JPEG_FILE_OK: return "OK";
    case JPEG_FILE_INVALID: return "INVALID";
    case JPEG_FILE_ALREADY_ATTEMPTED: return "ALREADY_ATTEMPTED";
    case JPEG_FILE_MEDIA_FAILED: return "MEDIA_FAILED";
    case JPEG_FILE_OPEN_FAILED: return "OPEN_FAILED";
    case JPEG_FILE_SIZE_FAILED: return "SIZE_FAILED";
    case JPEG_FILE_STREAM_FAILED: return "STREAM_FAILED";
    case JPEG_FILE_CLOSE_FAILED: return "CLOSE_FAILED";
    case JPEG_FILE_READ_FAILED: return "READ_FAILED";
    }
    return "UNKNOWN";
}
