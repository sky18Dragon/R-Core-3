#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Read Pico compiles out erase recovery: incompatible/full NVS requires migration.
// Legacy boards may retain their original erase-and-retry recovery policy.
esp_err_t photoframe_nvs_init(bool allow_erase);

#ifdef __cplusplus
}
#endif
