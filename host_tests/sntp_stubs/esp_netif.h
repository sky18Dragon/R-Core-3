#pragma once
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef esp_err_t (*esp_netif_callback_fn)(void *);
esp_err_t esp_netif_tcpip_exec(esp_netif_callback_fn, void *);
#ifdef __cplusplus
}
#endif
