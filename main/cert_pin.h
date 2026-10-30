#pragma once

#include <stddef.h>

#include "esp_err.h"

// Fetch the TLS cert from `url` (https://host[:port][/path]). Legacy boards
// store it via config_manager_set_ca_cert_der and touch the revision.
//
// On success returns ESP_OK.
// On failure returns an error and, if `err_out` is non-NULL and capacity is
// positive, writes a NUL-terminated diagnostic within `err_out_len` bytes.
// NULL/zero-capacity output is allowed and does not suppress the error result.
// Read Pico stores URL/certificate/ETag invalidation in one checked operation;
// its caller owns revision publication. RAM is unchanged on save failure, but
// a failed NVS write/commit does not imply persistent rollback.
esp_err_t cert_pin_fetch_and_store(const char *url, char *err_out, size_t err_out_len);

// Clear any pinned cert.
void cert_pin_clear(void);
