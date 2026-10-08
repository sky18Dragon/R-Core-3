#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    bool static_address;
    char ip[16], netmask[16], gateway[16], dns[16];
} wifi_network_t;
#define WIFI_NETWORK_PATCH_IP UINT32_C(1)
#define WIFI_NETWORK_PATCH_NETMASK UINT32_C(2)
#define WIFI_NETWORK_PATCH_GATEWAY UINT32_C(4)
#define WIFI_NETWORK_PATCH_DNS UINT32_C(8)
#define WIFI_NETWORK_PATCH_MODE UINT32_C(16)
#define WIFI_NETWORK_PATCH_ALL UINT32_C(31)
// Dotted decimal IPv4, four octets. Empty DNS means automatic (gateway for static).
// DHCP ignores retained static fields. No IO, truncation or output on failure.
bool wifi_network_valid(const wifi_network_t *network);
bool wifi_network_copy(wifi_network_t *out, bool static_address, const char *ip,
                       const char *netmask, const char *gateway, const char *dns);
// Apply supplied fields to a current snapshot, checking newly supplied IPv4
// text (empty retained static fields allowed for DHCP). No output on failure.
bool wifi_network_patch(wifi_network_t *out, const wifi_network_t *current,
                        uint32_t fields, const wifi_network_t *patch);
#ifdef __cplusplus
}
#endif
