#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "esp_err.h"

esp_err_t dns_server_start(void);
void dns_server_stop(void);
// Read Pico waits for worker-owned socket close/exit acknowledgement.
// Timeout or close uncertainty retains ownership and prevents restart.
esp_err_t dns_server_stop_checked(void);

#endif
