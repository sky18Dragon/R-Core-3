#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include "esp_err.h"

esp_err_t mdns_service_init(void);
esp_err_t mdns_service_update_hostname(void);
#if CONFIG_BOARD_DRIVER_READ_PICO
// Uses the explicit snapshot; never calls back into config manager.
esp_err_t mdns_service_update_hostname_named(const char *name);
#endif

#endif
