#include "download_file.h"
#include <string.h>
#include <strings.h>

typedef struct { const char *host; size_t size; unsigned port; bool tls; } origin_t;
static bool parse_origin(const char *url, origin_t *out)
{
    if (!url || strnlen(url, 2048) >= 2048) return false;
    const char *scheme = strstr(url, "://");
    if (!scheme) return false;
    out->tls = scheme - url == 5 && !strncasecmp(url, "https", 5);
    if (!out->tls && !(scheme - url == 4 && !strncasecmp(url, "http", 4))) return false;
    const char *host = scheme + 3;
    size_t n = strcspn(host, "/?#");
    if (!n || memchr(host, '@', n)) return false;
    for (size_t i = 0; i < n; ++i)
        if ((unsigned char)host[i] <= 32 || host[i] == '\\' || host[i] == 127) return false;
    const char *port = NULL;
    if (*host == '[') {
        const char *end = memchr(host, ']', n);
        if (!end || end == host + 1) return false;
        out->size = end - host + 1;
        if (out->size < n) {
            if (host[out->size] != ':') return false;
            port = host + out->size + 1;
        }
    } else {
        port = memchr(host, ':', n);
        out->size = port ? (size_t)(port++ - host) : n;
        if (!out->size) return false;
    }
    out->port = out->tls ? 443 : 80;
    if (port) {
        if (port == host + n) return false;
        unsigned value = 0;
        for (; port < host + n; ++port) {
            if (*port < '0' || *port > '9') return false;
            value = value * 10 + *port - '0';
            if (value > 65535) return false;
        }
        if (!value) return false;
        out->port = value;
    }
    out->host = host;
    return true;
}
bool download_same_origin(const char *a, const char *b)
{
    origin_t x, y;
    return parse_origin(a, &x) && parse_origin(b, &y) && x.tls == y.tls &&
           x.port == y.port && x.size == y.size && !strncasecmp(x.host, y.host, x.size);
}
bool download_redirect_same_origin(const char *origin, const char *location)
{
    origin_t parsed;
    if (!parse_origin(origin, &parsed) || !location || !*location ||
        strnlen(location, 2048) >= 2048) return false;
    if (strstr(location, "://")) return download_same_origin(origin, location);
    if (location[0] == '/' && location[1] == '/') return false;
    size_t first = strcspn(location, "/?#");
    if (memchr(location, ':', first)) return false;
    for (const unsigned char *p = (const unsigned char *)location; *p; ++p)
        if (*p <= 32 || *p == 127 || *p == '\\') return false;
    return true; // Relative references retain the already-authorized authority.
}

bool download_file_write(upload_file_t *owner, int status, const void *data,
                         int length, uint64_t elapsed_ms)
{
    if (!owner || owner->failed || owner->fd < 0)
        return false;
    if (length < 0 || elapsed_ms >= 120000) {
        owner->failed = true;
        return false;
    }
    // Redirect/error bodies must not contaminate the final 200 image.
    if (status != 200)
        return true;
    return upload_file_write(owner, data, (size_t)length);
}

download_result_t download_file_finish(upload_file_t *owner, bool transport_ok,
                                      int status, bool complete, int64_t content_length)
{
    if (!owner || owner->fd < 0 || owner->failed || !transport_ok ||
        (status != 200 && status != 304)) {
        upload_file_abort(owner);
        return DOWNLOAD_FAILED;
    }
    if (status == 304) {
        bool empty = owner->bytes == 0;
        upload_file_abort(owner); // Empty hidden staging is retained, never deleted.
        return empty && !owner->close_failed && owner->io.media_ready(owner->io.context)
                   ? DOWNLOAD_NOT_MODIFIED : DOWNLOAD_FAILED;
    }
    if (!complete || content_length < -1 ||
        (content_length >= 0 && (uint64_t)content_length != owner->bytes)) {
        upload_file_abort(owner);
        return DOWNLOAD_FAILED;
    }
    return upload_file_finish(owner, owner->bytes) ? DOWNLOAD_COMPLETE : DOWNLOAD_FAILED;
}
