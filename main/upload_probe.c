#include "upload_probe.h"

#include <errno.h>
#include <string.h>

static uint8_t pattern(size_t offset)
{
    return (uint8_t) ((offset * 37u + 11u) & 255u);
}

static bool verify(upload_probe_session_t *s, const upload_probe_io_t *io, upload_probe_result_t *r)
{
    void *c = io->file.context;
    if (!io->file.media_ready(c))
        return false;
    int fd = io->open_read(c, UPLOAD_PROBE_TARGET);
    if (fd < 0)
        return false;
    bool ok = true;
    uint8_t bytes[256];
    for (size_t offset = 0; ok && offset < 4096; offset += sizeof(bytes)) {
        if (!io->file.media_ready(c) || io->read(c, fd, bytes, sizeof(bytes)) != sizeof(bytes)) {
            ok = false;
            break;
        }
        for (size_t i = 0; i < sizeof(bytes); ++i)
            if (bytes[i] != pattern(offset + i)) {
                ok = false;
                break;
            }
        if (ok)
            r->verified += sizeof(bytes);
    }
    if (ok)
        ok = io->file.media_ready(c) && io->read(c, fd, bytes, 1) == 0;
    if (io->file.close(c, fd) != 0) {
        s->close_failed = true;
        io->file.close_uncertain(c);
        ok = false;
    }
    return ok && io->file.media_ready(c);
}

static bool stage_file(upload_probe_session_t *s, const upload_probe_io_t *io,
                       upload_probe_result_t *r, upload_file_t *f, const char *path, size_t count,
                       bool conflict)
{
    if (!upload_file_begin(f, &io->file, path, 4096))
        return false;
    ++r->created;
    // The diagnostic runs on the existing 6KiB main task, not an HTTP task.
    // Keep bounded 256-byte chunks and one file owner instead of 4KiB payload
    // plus two 512-byte-path transactions on that task's stack.
    uint8_t data[256];
    bool ok = true;
    for (size_t offset = 0; ok && offset < count;) {
        size_t n = count - offset < sizeof(data) ? count - offset : sizeof(data);
        for (size_t i = 0; i < n; ++i)
            data[i] = conflict ? 0xA5 : pattern(offset + i);
        ok = upload_file_write(f, data, n);
        if (ok)
            r->written += n;
        offset += n;
    }
    ok = ok && upload_file_finish(f, count);
    if (!ok)
        upload_file_abort(f);
    s->close_failed |= f->close_failed;
    return ok;
}

upload_probe_result_t upload_probe_run(upload_probe_session_t *s, const upload_probe_io_t *io)
{
    upload_probe_result_t r = {.stage = "invalid"};
    if (!s || !io || !io->target_absent || !io->open_read || !io->read || !io->file.media_ready ||
        !io->file.close || !io->file.close_uncertain)
        return r;
    if (s->attempted) {
        r.stage = "already-attempted";
        return r;
    }
    s->attempted = true;
    r.stage = "preflight";
    if (s->close_failed || !io->file.media_ready(io->file.context) ||
        !io->target_absent(io->file.context))
        return r;
    upload_file_t file = {.fd = -1};
    r.stage = "stage-main";
    if (!stage_file(s, io, &r, &file, UPLOAD_PROBE_STAGE, 4096, false))
        return r;
    r.stage = "publish-main";
    bool published = upload_file_publish(&file, UPLOAD_PROBE_STAGE, UPLOAD_PROBE_TARGET);
    r.published = file.published ? 1 : 0;
    if (!published)
        return r;
    r.stage = "verify-main";
    if (!verify(s, io, &r))
        return r;
    r.stage = "stage-conflict";
    if (!stage_file(s, io, &r, &file, UPLOAD_PROBE_CONFLICT, 16, true))
        return r;
    r.stage = "conflict";
    errno = 0;
    published = upload_file_publish(&file, UPLOAD_PROBE_CONFLICT, UPLOAD_PROBE_TARGET);
    if (published || file.published || errno != EEXIST)
        return r;
    r.conflict_rejected = true;
    r.stage = "verify-unchanged";
    if (!verify(s, io, &r))
        return r;
    r.ok = true;
    r.stage = "complete";
    return r;
}
