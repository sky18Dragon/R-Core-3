#pragma once
#include "lwip/ip_addr.h"
#define SNTP_OPMODE_POLL 0
#define SNTP_MAX_SERVERS 1
#ifdef __cplusplus
extern "C" {
#endif
void sntp_setoperatingmode(uint8_t);
void sntp_init(void);
void sntp_stop(void);
uint8_t sntp_enabled(void);
void sntp_setserver(uint8_t, const ip_addr_t *);
const ip_addr_t *sntp_getserver(uint8_t);
void sntp_setservername(uint8_t, const char *);
const char *sntp_getservername(uint8_t);
#ifdef __cplusplus
}
#endif
