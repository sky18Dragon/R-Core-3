#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
typedef struct { const char *key; const char *value; } mdns_txt_item_t;
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t mdns_init(void);
void mdns_free(void);
esp_err_t mdns_hostname_set(const char *hostname);
esp_err_t mdns_instance_name_set(const char *instance);
esp_err_t mdns_service_add(const char *instance, const char *service, const char *proto,
                           uint16_t port, mdns_txt_item_t *txt, size_t count);
#ifdef __cplusplus
}
#endif
