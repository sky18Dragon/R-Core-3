#include "wifi_text.h"
#include <string.h>

static int hex_byte(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool wifi_text_form_valid(const uint8_t *bytes, size_t length)
{
    static const char *keys[] = {"ssid", "password", "deviceName", "ipMode", "staticIp",
                                  "staticNetmask", "staticGateway", "dnsServer"};
    static const size_t limits[] = {32, 64, 63, 7, 15, 15, 15, 15};
    if (!bytes || !length || length > 511) return false;
    unsigned seen = 0;
    for (size_t at = 0; at < length;) {
        size_t end = at;
        while (end < length && bytes[end] != '&') ++end;
        size_t equal = at;
        while (equal < end && bytes[equal] != '=') {
            unsigned char c = bytes[equal];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
            ++equal;
        }
        if (equal == at || equal == end || equal - at > 64) return false;
        size_t limit = 511;
        for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
            if (strlen(keys[i]) == equal - at && !memcmp(keys[i], bytes + at, equal - at)) {
                if (seen & (1u << i)) return false;
                seen |= 1u << i; limit = limits[i]; break;
            }
        }
        size_t decoded = 0;
        for (size_t p = equal + 1; p < end; ++p) {
            unsigned char c = bytes[p];
            if (c == '%') {
                if (end - p < 3 || hex_byte(bytes[p + 1]) < 0 || hex_byte(bytes[p + 2]) < 0)
                    return false;
                c = (hex_byte(bytes[p + 1]) << 4) | hex_byte(bytes[p + 2]); p += 2;
            }
            if (c < 32 || c == 127 || ++decoded > limit) return false;
        }
        if (end == length) break;
        at = end + 1;
        if (at == length) return false;
    }
    return (seen & 1u) != 0;
}

bool wifi_text_credentials_valid(const char *ssid, const char *password)
{
    if (!ssid || !password) return false;
    size_t s = strnlen(ssid, 33), p = strnlen(password, 65);
    if (!s || s > 32 || p > 64 || (p && p < 8)) return false;
    for (size_t i = 0; i < s; ++i)
        if ((unsigned char)ssid[i] < 32 || (unsigned char)ssid[i] == 127) return false;
    for (size_t i = 0; i < p; ++i) {
        unsigned char c = password[i];
        if (c < 32 || c > 126 || (p == 64 && !strchr("0123456789abcdefABCDEF", c))) return false;
    }
    return true;
}

bool wifi_text_parse(const uint8_t *bytes, size_t length, wifi_text_t *out)
{
    if (!bytes || !length || length > WIFI_TEXT_MAX_BYTES || !out) return false;
    // Validate every byte, including ignored comments, before copying fields.
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = bytes[i];
        if (!c || c == 127 || (c < 32 && c != '\r' && c != '\n')) return false;
        if (c == '\r' && (i + 1 == length || bytes[i + 1] != '\n')) return false;
    }
    wifi_text_t parsed = {0};
    unsigned fields = 0;
    for (size_t at = 0; at < length;) {
        size_t end = at;
        while (end < length && bytes[end] != '\n') ++end;
        size_t count = end - at;
        if (count && bytes[at + count - 1] == '\r') --count;
        if (count && bytes[at] != '#') {
            char *dest = fields == 0 ? parsed.ssid : fields == 1 ? parsed.password : parsed.device_name;
            size_t capacity = fields == 0 ? sizeof(parsed.ssid) : fields == 1 ? sizeof(parsed.password)
                                                                          : sizeof(parsed.device_name);
            if (fields >= 3 || count >= capacity) return false;
            // Empty physical lines retain the old separator semantics. The
            // explicit marker is unambiguous (a two-byte WPA password is invalid).
            if (!(fields == 1 && count == 2 && bytes[at] == '"' && bytes[at + 1] == '"'))
                memcpy(dest, bytes + at, count);
            ++fields;
        }
        at = end < length ? end + 1 : end;
    }
    if (fields < 2 || !wifi_text_credentials_valid(parsed.ssid, parsed.password)) return false;
    parsed.has_device_name = fields == 3;
    *out = parsed;
    return true;
}
