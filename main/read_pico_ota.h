#pragma once
#include <stdbool.h>
#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#ifdef __cplusplus
extern "C" {
#endif
// Read Pico local-frame OTA: firmware uploaded from the local web page.
// Requires the appended-OTA partition table; on the factory-only table every
// entry point reports "not supported" and nothing is written. The factory
// app is never overwritten and stays the fallback.

// Call once, early in app_main after NVS is ready. Counts trial boots of a
// new, unconfirmed image and, after OTA_LOCAL_MAX_BOOT_TRIES, switches back to
// the previous image and restarts (does not return in that case).
void read_pico_ota_boot_guard(void);
// Call when the product is demonstrably up (HTTP serving, storage mounted).
// Confirms a pending image so later boots no longer count as trials.
void read_pico_ota_mark_healthy(void);
// Stream a raw image from the request body (application/octet-stream) into
// the next OTA slot, select it and schedule a restart. Sends the response.
esp_err_t read_pico_ota_receive(httpd_req_t *req);
// True while a firmware upload is being received or written.
bool read_pico_ota_busy(void);
// Adds supported/running/next/pending/version fields to object.
void read_pico_ota_describe(cJSON *object);
#ifdef __cplusplus
}
#endif
