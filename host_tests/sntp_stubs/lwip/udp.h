#pragma once
#include "lwip/ip_addr.h"
typedef uint8_t u8_t;
struct udp_pcb { unsigned placeholder; };
#define IPADDR_TYPE_ANY 0
#ifdef __cplusplus
extern "C" {
#endif
struct udp_pcb *udp_new_ip_type(u8_t);
void udp_remove(struct udp_pcb *);
#ifdef __cplusplus
}
#endif
