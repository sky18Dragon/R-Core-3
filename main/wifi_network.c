#include "wifi_network.h"
#include <stddef.h>
#include <string.h>

static bool ipv4(const char *s)
{
    size_t n = strnlen(s, 16), at = 0;
    if (!n || n == 16) return false;
    for (unsigned octet = 0; octet < 4; ++octet) {
        // The fixed lwIP conversion also accepts octal-style leading zeroes.
        // Reject those spellings so validation and SDK application agree.
        size_t first = at;
        unsigned value = 0, digits = 0;
        while (at < n && s[at] >= '0' && s[at] <= '9') {
            value = value * 10 + (unsigned)(s[at++] - '0');
            if (++digits > 3 || value > 255) return false;
        }
        if (!digits || (digits > 1 && s[first] == '0') ||
            (octet < 3 && (at == n || s[at++] != '.'))) return false;
    }
    return at == n;
}
bool wifi_network_valid(const wifi_network_t *network)
{
    if (!network || strnlen(network->ip, 16) == 16 || strnlen(network->netmask, 16) == 16 ||
        strnlen(network->gateway, 16) == 16 || strnlen(network->dns, 16) == 16) return false;
    return (!network->static_address ||
            (ipv4(network->ip) && ipv4(network->netmask) && ipv4(network->gateway))) &&
           (!network->dns[0] || ipv4(network->dns));
}
bool wifi_network_copy(wifi_network_t *out, bool static_address, const char *ip,
                       const char *netmask, const char *gateway, const char *dns)
{
    if (!out || !ip || !netmask || !gateway || !dns) return false;
    const char *values[] = {ip, netmask, gateway, dns};
    wifi_network_t value = {.static_address = static_address};
    char *dest[] = {value.ip, value.netmask, value.gateway, value.dns};
    for (unsigned i = 0; i < 4; ++i) {
        size_t n = strnlen(values[i], 16);
        if (n == 16) return false;
        memcpy(dest[i], values[i], n + 1);
    }
    if (!wifi_network_valid(&value)) return false;
    *out = value; return true;
}

bool wifi_network_patch(wifi_network_t *out, const wifi_network_t *current,
                        uint32_t fields, const wifi_network_t *patch)
{
    if (!out || !current || !patch || fields & ~WIFI_NETWORK_PATCH_ALL) return false;
    wifi_network_t candidate = *current;
    const char *source[] = {patch->ip, patch->netmask, patch->gateway, patch->dns};
    char *destination[] = {candidate.ip, candidate.netmask, candidate.gateway, candidate.dns};
    for (unsigned i = 0; i < 4; ++i) {
        if (!(fields & (UINT32_C(1) << i))) continue;
        size_t length = strnlen(source[i], 16);
        if (length == 16 || (length && !ipv4(source[i]))) return false;
        memset(destination[i], 0, 16);
        memcpy(destination[i], source[i], length + 1);
    }
    if (fields & WIFI_NETWORK_PATCH_MODE) candidate.static_address = patch->static_address;
    if (!wifi_network_valid(&candidate)) return false;
    *out = candidate;
    return true;
}
