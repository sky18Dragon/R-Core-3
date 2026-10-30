#pragma once
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
#define LWIP_UDP 1
#define LWIP_DNS 1
#define LWIP_DHCP 0
#define SNTP_SERVER_DNS 1
#define SNTP_SUPPORT_MULTIPLE_SERVERS 0
#define SNTP_MONITOR_SERVER_REACHABILITY 1
#define SNTP_GET_SERVERS_FROM_DHCP 0
#define SNTP_GET_SERVERS_FROM_DHCPV6 0
#define SNTP_RETRY_TIMEOUT_EXP 1
#define SNTP_RETRY_TIMEOUT 15000
#define SNTP_RETRY_TIMEOUT_MAX 150000
#define SNTP_RECV_TIMEOUT 15000
#define SNTP_UPDATE_DELAY 3600000
#define SNTP_STARTUP_DELAY 1
#define SNTP_STARTUP_DELAY_FUNC 100
#define SNTP_PORT 123
#define SNTP_DEBUG 0
#define LWIP_DBG_TRACE 0
#define LWIP_DBG_STATE 0
#define LWIP_DBG_LEVEL_WARNING 0
#define LWIP_DBG_LEVEL_SERIOUS 0
#define LWIP_DEBUGF(a, b) ((void)0)
#define LWIP_ASSERT(message, condition) assert(condition)
#define LWIP_ASSERT_CORE_LOCKED() ((void)0)
#define LWIP_UNUSED_ARG(a) ((void)(a))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#define PACK_STRUCT_FLD_8(x) x
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT
#define lwip_htonl htonl
#define lwip_ntohl ntohl
#define SNTP_GET_SYSTEM_TIME(s, us) __wrap_sntp_get_system_time(&(s), &(us))
#define SNTP_SET_SYSTEM_TIME_US(s, us) sntp_wire_set_time((s), (us))
#ifdef __cplusplus
extern "C" {
#endif
void __wrap_sntp_get_system_time(uint32_t *, uint32_t *);
void sntp_wire_set_time(uint32_t, uint32_t);
#ifdef __cplusplus
}
#endif
