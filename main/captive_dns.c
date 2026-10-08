#include "captive_dns.h"
#include <string.h>

static uint16_t read16(const uint8_t *p) { return (uint16_t)((uint16_t)p[0] << 8 | p[1]); }
static void write16(uint8_t *p, uint16_t value) { p[0] = value >> 8; p[1] = value & 255; }

bool captive_dns_response(const uint8_t *query, size_t length, const uint8_t ipv4[4],
                          uint8_t *output, size_t capacity, size_t *written)
{
    if (!query || !ipv4 || !output || !written || length < 17 || length > CAPTIVE_DNS_MAX_PACKET)
        return false;
    uint16_t flags = read16(query + 2);
    // Standard query only; allow RD/AD/CD, never copy authentication/recursion claims.
    if ((flags & ~0x0130u) || read16(query + 4) != 1 || read16(query + 6) ||
        read16(query + 8) || read16(query + 10) > 1) return false;
    size_t at = 12;
    while (at < length) {
        unsigned label = query[at++];
        if (!label) break;
        if (label > 63 || label > length - at) return false;
        at += label;
    }
    if (at == 12 || query[at - 1] != 0 || at - 12 > 255 || length - at < 4)
        return false;
    uint16_t type = read16(query + at);
    if (read16(query + at + 2) != 1) return false;
    size_t question_end = at + 4;
    bool edns = read16(query + 10) == 1;
    bool bad_version = false;
    uint8_t do_flag = 0;
    if (edns) {
        at = question_end;
        if (length - at < 11 || query[at] || read16(query + at + 1) != 41 || query[at + 5])
            return false;
        bad_version = query[at + 6] != 0;
        do_flag = query[at + 7] & 0x80;
        size_t rdata = read16(query + at + 9);
        at += 11;
        if (rdata != length - at) return false;
        while (at < length) {
            if (length - at < 4) return false;
            size_t option = read16(query + at + 2);
            at += 4;
            if (option > length - at) return false;
            at += option;
        }
    } else if (question_end != length) return false;
    bool answer = type == 1 && !bad_version;
    size_t size = question_end + (answer ? 16u : 0u) + (edns ? 11u : 0u);
    if (size > capacity || size > CAPTIVE_DNS_MAX_PACKET) return false;
    uint8_t response[CAPTIVE_DNS_MAX_PACKET] = {0};
    memcpy(response, query, question_end);
    write16(response + 2, 0x8000u | (flags & 0x0100u));
    write16(response + 6, answer ? 1 : 0);
    write16(response + 8, 0);
    write16(response + 10, edns ? 1 : 0);
    at = question_end;
    if (answer) {
        const uint8_t record[12] = {0xc0, 0x0c, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4};
        memcpy(response + at, record, sizeof(record));
        memcpy(response + at + 12, ipv4, 4);
        at += 16;
    }
    if (edns) {
        const uint8_t opt[11] = {0, 0, 41, 2, 0, 0, 0, 0, 0, 0, 0};
        memcpy(response + at, opt, sizeof(opt));
        response[at + 5] = bad_version ? 1 : 0; // extended RCODE=16 (BADVERS)
        response[at + 7] = do_flag;
    }
    memcpy(output, response, size);
    *written = size;
    return true;
}
