#include "upload_stream.h"

#include <string.h>
#include <strings.h>

enum { INITIAL, HEADER, BODY, SUFFIX, DONE, FAILED };

bool upload_leaf_safe(const char *s)
{
    if (!s || !*s || *s == '.' || strlen(s) > UPLOAD_FILENAME_MAX)
        return false;
    size_t n = strlen(s);
    if (s[n - 1] == '.' || s[n - 1] == ' ')
        return false;
    for (const unsigned char *p = (const unsigned char *) s; *p; ++p)
        if (*p < 32 || *p == 127 || strchr("/\\:*?\"<>|", *p))
            return false;
    return true;
}

static bool boundary_safe(const char *s)
{
    size_t n = s ? strlen(s) : 0;
    if (!n || n > UPLOAD_BOUNDARY_MAX)
        return false;
    for (size_t i = 0; i < n; ++i)
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') ||
              (s[i] >= '0' && s[i] <= '9') || strchr("'()+_,-./:=?", s[i])))
            return false;
    return true;
}

bool upload_boundary_from_content_type(const char *type, char *out, size_t cap)
{
    const char *media = "multipart/form-data";
    size_t n = strlen(media);
    if (!type || !out || strncasecmp(type, media, n) || type[n] != ';')
        return false;
    const char *p = type + n + 1;
    while (*p == ' ' || *p == '\t')
        ++p;
    if (strncasecmp(p, "boundary=", 9))
        return false;
    p += 9;
    bool quoted = *p == '"';
    if (quoted)
        ++p;
    const char *start = p;
    while (*p && (quoted ? *p != '"' : *p != ';' && *p != ' ' && *p != '\t'))
        ++p;
    n = (size_t) (p - start);
    if (quoted && *p++ != '"')
        return false;
    while (*p == ' ' || *p == '\t')
        ++p;
    if (*p || !n || n > UPLOAD_BOUNDARY_MAX || n >= cap)
        return false;
    memcpy(out, start, n);
    out[n] = 0;
    return boundary_safe(out);
}

bool upload_stream_init(upload_stream_t *s, const char *b, size_t limit,
                        const upload_stream_sink_t *sink)
{
    if (!s || !boundary_safe(b) || !limit || !sink || !sink->begin || !sink->write || !sink->end)
        return false;
    memset(s, 0, sizeof(*s));
    s->sink = *sink;
    s->limit = limit;
    memcpy(s->marker, "\r\n--", 4);
    strcpy(s->marker + 4, b);
    s->marker_size = strlen(s->marker);
    return true;
}

static bool flush(upload_stream_t *s)
{
    if (s->output_size && !s->sink.write(s->sink.context, s->output, s->output_size))
        return false;
    s->output_size = 0;
    return true;
}

static bool emit(upload_stream_t *s, uint8_t b)
{
    s->output[s->output_size++] = b;
    return s->output_size != sizeof(s->output) || flush(s);
}

static bool headers(upload_stream_t *s)
{
    char field[16] = {0}, filename[UPLOAD_FILENAME_MAX + 1] = {0};
    bool disposition = false, name_seen = false, file_seen = false;
    char *line = s->header;
    while (*line) {
        char *end = strstr(line, "\r\n");
        if (!end)
            return false;
        *end = 0;
        if (strncasecmp(line, "Content-Disposition:", 20) == 0) {
            if (disposition)
                return false;
            disposition = true;
            char *p = line + 20;
            while (*p == ' ' || *p == '\t')
                ++p;
            if (strncasecmp(p, "form-data;", 10))
                return false;
            p += 10;
            while (*p) {
                while (*p == ' ' || *p == '\t')
                    ++p;
                char *key = p;
                while (*p && *p != '=')
                    ++p;
                if (!*p)
                    return false;
                *p++ = 0;
                if (*p++ != '"')
                    return false;
                char *value = p;
                while (*p && *p != '"')
                    ++p;
                if (!*p)
                    return false;
                *p++ = 0;
                if (!strcasecmp(key, "name")) {
                    if (name_seen || strlen(value) >= sizeof(field))
                        return false;
                    name_seen = true;
                    strcpy(field, value);
                } else if (!strcasecmp(key, "filename")) {
                    if (file_seen || !upload_leaf_safe(value))
                        return false;
                    file_seen = true;
                    strcpy(filename, value);
                } else
                    return false;
                while (*p == ' ' || *p == '\t')
                    ++p;
                if (*p && *p++ != ';')
                    return false;
            }
        } else if (strncasecmp(line, "Content-Type:", 13) != 0) {
            return false;
        }
        line = end + 2;
    }
    unsigned bit = !strcmp(field, "image") ? 1 : !strcmp(field, "thumbnail") ? 2 : 0;
    if (!disposition || !name_seen || !file_seen || !bit || (s->seen & bit))
        return false;
    s->seen |= bit;
    return s->sink.begin(s->sink.context, field, filename);
}

static bool body_byte(upload_stream_t *s, uint8_t b)
{
    s->pending[s->pending_size++] = b;
    while (s->pending_size && memcmp(s->pending, s->marker, s->pending_size)) {
        if (!emit(s, s->pending[0]))
            return false;
        memmove(s->pending, s->pending + 1, --s->pending_size);
    }
    if (s->pending_size == s->marker_size) {
        s->state = SUFFIX;
        s->suffix_size = 0;
    }
    return true;
}

static bool byte(upload_stream_t *s, uint8_t b)
{
    switch (s->state) {
    case INITIAL: {
        // Initial delimiter has no leading CRLF, followed by CRLF.
        size_t n = s->marker_size - 2;
        uint8_t expected = s->pending_size < n    ? (uint8_t) s->marker[s->pending_size + 2]
                           : s->pending_size == n ? '\r'
                                                  : '\n';
        if (b != expected)
            return false;
        if (++s->pending_size == n + 2) {
            s->pending_size = 0;
            s->state = HEADER;
        }
        return true;
    }
    case HEADER:
        if (!b || (b < 32 && b != '\r' && b != '\n' && b != '\t') || b == 127 ||
            s->header_size == UPLOAD_HEADER_MAX)
            return false;
        s->header[s->header_size++] = (char) b;
        s->header[s->header_size] = 0;
        if (s->header_size >= 4 && !memcmp(s->header + s->header_size - 4, "\r\n\r\n", 4)) {
            s->header[s->header_size - 2] = 0;
            if (!headers(s))
                return false;
            s->header_size = 0;
            s->state = BODY;
        }
        return true;
    case BODY:
        return body_byte(s, b);
    case SUFFIX:
        s->suffix[s->suffix_size++] = b;
        if (s->suffix_size < 2)
            return true;
        if (!memcmp(s->suffix, "\r\n", 2) || !memcmp(s->suffix, "--", 2)) {
            if (!flush(s) || !s->sink.end(s->sink.context))
                return false;
            s->state = s->suffix[0] == '-' ? DONE : HEADER;
            s->pending_size = 0;
            return true;
        }
        // A delimiter-looking sequence with a non-delimiter suffix is payload.
        for (size_t i = 0; i < s->pending_size; ++i)
            if (!emit(s, s->pending[i]))
                return false;
        s->pending_size = 0;
        s->state = BODY;
        return body_byte(s, s->suffix[0]) && body_byte(s, s->suffix[1]);
    case DONE:
        if (s->trailing >= 2 || b != (s->trailing == 0 ? '\r' : '\n'))
            return false;
        ++s->trailing;
        return true;
    default:
        return false;
    }
}

bool upload_stream_feed(upload_stream_t *s, const void *data, size_t n)
{
    if (!s || s->state == FAILED)
        return false;
    if ((!data && n) || n > s->limit - s->total) {
        s->state = FAILED;
        return false;
    }
    s->total += n;
    const uint8_t *p = data;
    for (size_t i = 0; i < n; ++i)
        if (!byte(s, p[i])) {
            s->state = FAILED;
            return false;
        }
    if (!flush(s)) {
        s->state = FAILED;
        return false;
    }
    return true;
}

bool upload_stream_finish(upload_stream_t *s)
{
    if (!s)
        return false;
    bool ok = s->state == DONE && s->seen && s->trailing != 1;
    if (!ok)
        s->state = FAILED;
    return ok;
}
