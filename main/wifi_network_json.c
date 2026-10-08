#include "wifi_network_json.h"
#include <string.h>
#include <strings.h>

esp_err_t wifi_network_json_patch(const cJSON *root, uint32_t *fields, wifi_network_t *patch)
{
    if (!cJSON_IsObject(root) || !fields || !patch) return ESP_ERR_INVALID_ARG;
    const char *keys[] = {"static_ip", "static_netmask", "static_gateway", "dns_server", "ip_mode"};
    wifi_network_t candidate = {0};
    char *destinations[] = {candidate.ip, candidate.netmask, candidate.gateway, candidate.dns};
    uint32_t present = 0;
    for (const cJSON *item = root->child; item; item = item->next) {
        if (!item->string) return ESP_ERR_INVALID_ARG;
        for (unsigned i = 0; i < 5; ++i) {
            if (strcasecmp(item->string, keys[i])) continue;
            uint32_t bit = UINT32_C(1) << i;
            if ((present & bit) || !cJSON_IsString(item) || !item->valuestring) return ESP_ERR_INVALID_ARG;
            present |= bit;
            if (i == 4) {
                if (strcmp(item->valuestring, "static") && strcmp(item->valuestring, "dhcp")) return ESP_ERR_INVALID_ARG;
                candidate.static_address = !strcmp(item->valuestring, "static");
            } else {
                size_t length = strnlen(item->valuestring, 16);
                if (length == 16) return ESP_ERR_INVALID_SIZE;
                memcpy(destinations[i], item->valuestring, length + 1);
            }
        }
    }
    *fields = present;
    *patch = candidate;
    return ESP_OK;
}
