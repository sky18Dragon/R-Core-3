#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    char target[33];
    uint8_t bssid[6];
    bool active, associated, ready;
} wifi_link_t;
void wifi_link_cancel(wifi_link_t *);
bool wifi_link_arm(wifi_link_t *, const char *ssid);
bool wifi_link_associated(wifi_link_t *, const uint8_t *ssid, size_t length, const uint8_t *bssid);
// Caller supplies current driver association and a validated current netif IP,
// not just the cached CONNECTED bit. SDK events have no application attempt ID;
// queue fencing and live association checks belong to the manager adapter.
bool wifi_link_ip(wifi_link_t *, const uint8_t *ssid, size_t length, const uint8_t *bssid,
                  bool current_ip_valid);
void wifi_link_disconnected(wifi_link_t *);
#ifdef __cplusplus
}
#endif
