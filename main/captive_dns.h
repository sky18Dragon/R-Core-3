#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CAPTIVE_DNS_MAX_PACKET 512
// Bounded captive-portal subset, not a recursive/general DNS implementation.
// One uncompressed IN question, optional single root OPT with checked options.
// A gets the supplied IPv4; other types get no answer. EDNS replies include OPT;
// unsupported EDNS versions get BADVERS. Invalid/unsupported input changes no output.
bool captive_dns_response(const uint8_t *query, size_t length, const uint8_t ipv4[4],
                          uint8_t *output, size_t capacity, size_t *written);
#ifdef __cplusplus
}
#endif
