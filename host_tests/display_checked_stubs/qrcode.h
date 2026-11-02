#pragma once
// Host fake of the espressif__qrcode API used by display_manager.c; the test
// file defines the functions and records every encoded text.
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef const void *esp_qrcode_handle_t;
enum { ESP_QRCODE_ECC_LOW, ESP_QRCODE_ECC_MED, ESP_QRCODE_ECC_QUART, ESP_QRCODE_ECC_HIGH };
typedef struct {
    void (*display_func)(esp_qrcode_handle_t qrcode);
    int max_qrcode_version;
    int qrcode_ecc_level;
} esp_qrcode_config_t;
esp_err_t esp_qrcode_generate(esp_qrcode_config_t *cfg, const char *text);
int esp_qrcode_get_size(esp_qrcode_handle_t qrcode);
bool esp_qrcode_get_module(esp_qrcode_handle_t qrcode, int x, int y);
#ifdef __cplusplus
}
#endif
