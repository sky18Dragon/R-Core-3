#include "wifi_link.h"
#include <string.h>
void wifi_link_cancel(wifi_link_t *link) { memset(link, 0, sizeof(*link)); }
bool wifi_link_arm(wifi_link_t *link, const char *ssid)
{
    wifi_link_cancel(link);
    if (!ssid) return false;
    size_t n = strnlen(ssid, sizeof(link->target));
    if (!n || n >= sizeof(link->target)) return false;
    memcpy(link->target, ssid, n + 1); link->active = true;
    return true;
}
static bool target(const wifi_link_t *link, const uint8_t *ssid, size_t n)
{
    return link->active && ssid && n && n <= 32 && strlen(link->target) == n &&
           !memcmp(link->target, ssid, n);
}
bool wifi_link_associated(wifi_link_t *link, const uint8_t *ssid, size_t n, const uint8_t *bssid)
{
    if (!target(link, ssid, n) || !bssid) return false;
    static const uint8_t zero[6];
    if (!memcmp(bssid, zero, sizeof(zero))) return false;
    memcpy(link->bssid, bssid, sizeof(link->bssid));
    link->associated = true; link->ready = false; return true;
}
bool wifi_link_ip(wifi_link_t *link, const uint8_t *ssid, size_t n, const uint8_t *bssid, bool valid)
{
    if (!link->associated || !valid || !target(link, ssid, n) || !bssid ||
        memcmp(link->bssid, bssid, sizeof(link->bssid))) return false;
    link->ready = true; return true;
}
void wifi_link_disconnected(wifi_link_t *link)
{
    link->associated = link->ready = false;
    memset(link->bssid, 0, sizeof(link->bssid));
}
