#pragma once
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/dns.h"
struct udp_pcb { unsigned placeholder; };
typedef void (*udp_recv_fn)(void *, struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t);
#define IPADDR_TYPE_ANY 0
#define SOF_BROADCAST 0
#define SNTP_OPMODE_LISTENONLY 1
#define IP_ANY_TYPE (&sntp_wire_any)
#define IP_ADDR_ANY (&sntp_wire_any)
#define ip_set_option(a, b) ((void)0)
#define ip_addr_eq(a, b) ((a)->value == (b)->value)
#define ip_addr_isany_val(a) ((a).value == 0)
#define ip_addr_set_zero(a) ((a)->value = 0)
#define ip_addr_copy(a, b) ((a) = (b))
#define ip_addr_set_zero_ip4(a) ((a)->value = 0)
#ifdef __cplusplus
extern "C" {
#endif
extern const ip_addr_t sntp_wire_any;
struct udp_pcb *udp_new_ip_type(u8_t);
void udp_recv(struct udp_pcb *, udp_recv_fn, void *);
err_t udp_sendto(struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t);
err_t udp_bind(struct udp_pcb *, const ip_addr_t *, u16_t);
void udp_remove(struct udp_pcb *);
#ifdef __cplusplus
}
#endif
