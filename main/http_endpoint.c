#include "http_endpoint.h"
#include "http_parser.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

bool http_endpoint_host_header(const char *url, char *out, size_t capacity)
{
    http_endpoint_t endpoint;
    if (!out || !capacity || !http_endpoint_parse(url, &endpoint)) return false;
    char value[264]; // 255 host + brackets + colon + 5 port digits + NUL.
    bool ipv6 = strchr(endpoint.host, ':') != NULL;
    bool default_port = endpoint.port == (endpoint.tls ? 443u : 80u);
    int length = default_port ? snprintf(value, sizeof(value), ipv6 ? "[%s]" : "%s", endpoint.host) :
        snprintf(value, sizeof(value), ipv6 ? "[%s]:%u" : "%s:%u", endpoint.host, endpoint.port);
    if (length < 0 || (size_t)length >= sizeof(value) || (size_t)length >= capacity) return false;
    memcpy(out, value, (size_t)length + 1);
    return true;
}

bool http_redirect_reference_valid(const char *reference)
{
    if (!reference) return false;
    size_t length = strnlen(reference, 2048);
    if (!length || length == 2048) return false;
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)reference[i] <= 32 || (unsigned char)reference[i] == 127 ||
            reference[i] == '\\') return false;
    if (reference[0] == '/' && reference[1] == '/') {
        // Prefix only for validation. The caller must retain the original
        // reference and the SDK remains responsible for redirect resolution.
        char *absolute = malloc(length + 6);
        if (!absolute) return false;
        memcpy(absolute, "http:", 5);
        memcpy(absolute + 5, reference, length + 1);
        bool valid = http_endpoint_valid(absolute);
        free(absolute);
        return valid;
    }
    size_t first = strcspn(reference, "/?#");
    if (memchr(reference, ':', first)) return http_endpoint_valid(reference);
    return true; // No authority/scheme is introduced by a relative reference.
}

bool http_endpoint_valid(const char *url)
{
    http_endpoint_t endpoint;
    return http_endpoint_parse(url, &endpoint);
}

bool http_endpoint_parse(const char *url, http_endpoint_t *out)
{
    if (!url || !out) return false;
    size_t length = strnlen(url, 2048);
    if (!length || length == 2048) return false;
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)url[i] <= 32 || (unsigned char)url[i] == 127 || url[i] == '\\') return false;
    struct http_parser_url parsed;
    http_parser_url_init(&parsed);
    if (http_parser_parse_url(url, length, 0, &parsed) != 0 ||
        !(parsed.field_set & (1 << UF_SCHEMA)) || !(parsed.field_set & (1 << UF_HOST)) ||
        (parsed.field_set & (1 << UF_USERINFO))) return false;
    const char *scheme = url + parsed.field_data[UF_SCHEMA].off;
    unsigned scheme_length = parsed.field_data[UF_SCHEMA].len;
    bool tls = scheme_length == 5 && !strncasecmp(scheme, "https", 5);
    if (!tls && !(scheme_length == 4 && !strncasecmp(scheme, "http", 4))) return false;
    unsigned host_length = parsed.field_data[UF_HOST].len;
    if (!host_length || host_length >= sizeof(out->host)) return false;
    unsigned port = (parsed.field_set & (1 << UF_PORT)) ? parsed.port : tls ? 443 : 80;
    if (!port) return false;
    http_endpoint_t candidate = {.port = port, .tls = tls};
    memcpy(candidate.host, url + parsed.field_data[UF_HOST].off, host_length);
    *out = candidate;
    return true;
}
