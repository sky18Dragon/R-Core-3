#pragma once
#include "lwip/ip_addr.h"
typedef int err_t;
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_INPROGRESS -5
#define ERR_ARG -16
#define ERR_TIMEOUT -3
#define LWIP_DNS_ADDRTYPE_IPV4 0
#define LWIP_DNS_ADDRTYPE_IPV4_IPV6 2
#define LWIP_DNS_ADDRTYPE_DEFAULT LWIP_DNS_ADDRTYPE_IPV4_IPV6
typedef void (*dns_found_callback)(const char *, const ip_addr_t *, void *);
#ifdef __cplusplus
extern "C" {
#endif
err_t dns_gethostbyname_addrtype(const char *, ip_addr_t *, dns_found_callback, void *, uint8_t);
err_t dns_gethostbyname(const char *, ip_addr_t *, dns_found_callback, void *);
#ifdef __cplusplus
}
#endif
