#include "upload_file.h"

#include <limits.h>
#include <string.h>

bool upload_file_begin(upload_file_t *f, const upload_file_io_t *io, const char *path, size_t limit)
{
    if (!f || !io || !path || !*path || strlen(path) >= sizeof(f->source) || !limit ||
        !io->media_ready || !io->create_new || !io->write || !io->sync || !io->close ||
        !io->close_uncertain || !io->publish_no_replace)
        return false;
    memset(f, 0, sizeof(*f));
    f->io = *io;
    strcpy(f->source, path);
    f->fd = -1;
    f->limit = limit;
    if (!io->media_ready(io->context)) {
        f->failed = true;
        return false;
    }
    f->fd = io->create_new(io->context, path);
    f->failed = f->fd < 0;
    return !f->failed;
}

bool upload_file_write(upload_file_t *f, const uint8_t *data, size_t n)
{
    if (!f || f->failed || f->ready || f->fd < 0)
        return false;
    if ((!data && n) || n > INT_MAX || n > f->limit - f->bytes ||
        !f->io.media_ready(f->io.context)) {
        f->failed = true;
        return false;
    }
    if (n && f->io.write(f->io.context, f->fd, data, n) != (int) n) {
        f->failed = true;  // Short/zero/error writes are terminal; never pretend complete.
        return false;
    }
    f->bytes += n;
    if (!f->io.media_ready(f->io.context)) {
        f->failed = true;
        return false;
    }
    return true;
}

static bool close_once(upload_file_t *f)
{
    int fd = f->fd;
    f->fd = -1;  // A failed close is ambiguous; never retry/reuse this descriptor.
    if (fd < 0)
        return !f->close_failed;
    if (f->io.close(f->io.context, fd) == 0)
        return true;
    f->close_failed = f->failed = true;
    f->io.close_uncertain(f->io.context);
    return false;
}

void upload_file_abort(upload_file_t *f)
{
    if (!f)
        return;
    f->failed = true;
    f->ready = false;
    if (f->fd >= 0)
        (void) close_once(f);
}

bool upload_file_finish(upload_file_t *f, size_t expected)
{
    if (!f || f->ready || f->published || f->fd < 0)
        return false;
    if (f->failed || !f->bytes || (expected && expected != f->bytes) ||
        !f->io.media_ready(f->io.context) || f->io.sync(f->io.context, f->fd) != 0)
        f->failed = true;
    bool closed = close_once(f);
    if (!closed || f->failed || !f->io.media_ready(f->io.context)) {
        f->failed = true;
        return false;
    }
    f->ready = true;
    return true;
}

bool upload_file_publish(upload_file_t *f, const char *source, const char *destination)
{
    if (!f || !f->ready || f->failed || f->published || !source || !*source || !destination ||
        !*destination || strcmp(source, f->source) || !strcmp(source, destination))
        return false;
    if (!f->io.media_ready(f->io.context) ||
        f->io.publish_no_replace(f->io.context, source, destination) != 0) {
        f->failed = true;
        return false;
    }
    f->published = true;
    // This can fail after rename: caller must report failure, not delete/retry
    // or claim rollback. Publication is not a power-loss durability guarantee.
    if (!f->io.media_ready(f->io.context)) {
        f->failed = true;
        return false;
    }
    return true;
}
