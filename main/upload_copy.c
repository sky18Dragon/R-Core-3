#include "upload_copy.h"

bool upload_copy_finish(upload_file_t *f, const upload_copy_source_t *source, size_t expected)
{
    if (!source || !source->read || !source->close) {
        upload_file_abort(f);
        return false;
    }
    bool ok = f && f->fd >= 0 && !f->failed && !f->bytes && expected && expected <= f->limit;
    size_t remaining = expected;
    uint8_t bytes[256];
    while (ok && remaining) {
        size_t count = remaining < sizeof(bytes) ? remaining : sizeof(bytes);
        ok = f->io.media_ready(f->io.context);
        int n = ok ? source->read(source->context, bytes, count) : -1;
        ok = n == (int)count && upload_file_write(f, bytes, count);
        if (ok) {
            size_t previous = expected - remaining;
            remaining -= count;
            if (source->yield && (previous >> 15) != ((expected - remaining) >> 15))
                source->yield(source->context);
        }
    }
    if (ok)
        ok = f->io.media_ready(f->io.context) && source->read(source->context, bytes, 1) == 0;
    if (source->close(source->context) != 0) {
        if (f && f->io.close_uncertain) f->io.close_uncertain(f->io.context);
        ok = false;
    }
    if (!ok) {
        upload_file_abort(f);
        return false;
    }
    return upload_file_finish(f, expected);
}
