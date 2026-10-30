#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WIFI_TEXT_MAX_BYTES 1024
typedef struct {
    char ssid[33];       // 32 wire bytes plus C terminator.
    char password[65];   // 63-byte passphrase or 64 hex PSK plus terminator.
    char device_name[64];
    bool has_device_name;
} wifi_text_t;
// Legacy positional wifi.txt: blank/comment lines skipped; two fields plus
// optional device name. LF/CRLF; no truncation/NUL/control bytes/extra fields.
// A password line consisting of "" explicitly selects an open network.
// Output is changed only on complete validation; no file/NVS/board operations.
bool wifi_text_parse(const uint8_t *bytes, size_t length, wifi_text_t *out);
bool wifi_text_credentials_valid(const char *ssid, const char *password);
// Validate the complete URL-encoded setup body before legacy field extraction:
// known field capacities, unique known keys and exact percent escapes, no NUL.
bool wifi_text_form_valid(const uint8_t *bytes, size_t length);
#ifdef __cplusplus
}
#endif
