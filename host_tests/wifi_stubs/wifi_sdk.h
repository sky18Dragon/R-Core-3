#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#define ESP_ERROR_CHECK(call) do { if ((call) != ESP_OK) abort(); } while (0)
#define ESP_ERR_WIFI_NOT_STARTED 0x3002
#define ESP_ERR_WIFI_NOT_INIT 0x3001
#define ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED 0x5005
#define ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED 0x5004
#define ESP_IPADDR_TYPE_V4 0
#define ESP_NETIF_DNS_MAIN 0
#define WIFI_ALL_CHANNEL_SCAN 0
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(ip) (unsigned)((ip)->addr&255), (unsigned)(((ip)->addr>>8)&255), \
                  (unsigned)(((ip)->addr>>16)&255), (unsigned)(((ip)->addr>>24)&255)
typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void *, esp_event_base_t, int32_t, void *);
#define ESP_EVENT_ANY_ID -1
#define ESP_EVENT_DEFINE_BASE(name) esp_event_base_t name = #name
extern esp_event_base_t WIFI_EVENT, IP_EVENT;
enum { WIFI_EVENT_STA_START=1, WIFI_EVENT_STA_CONNECTED, WIFI_EVENT_STA_DISCONNECTED, WIFI_EVENT_STA_STOP };
enum { IP_EVENT_STA_GOT_IP=10, IP_EVENT_GOT_IP6, IP_EVENT_STA_LOST_IP };
enum { WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT=15, WIFI_REASON_AUTH_FAIL=202, WIFI_REASON_HANDSHAKE_TIMEOUT=204 };
typedef enum { WIFI_MODE_NULL, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA } wifi_mode_t;
enum { WIFI_IF_STA, WIFI_IF_AP, WIFI_PS_NONE, WIFI_PS_MIN_MODEM, WIFI_AUTH_OPEN=0, WIFI_AUTH_WPA2_PSK=3 };
enum { WIFI_STORAGE_FLASH, WIFI_STORAGE_RAM };
typedef struct { uint32_t addr; } esp_ip4_addr_t;
typedef struct { esp_ip4_addr_t ip, netmask, gw; } esp_netif_ip_info_t;
typedef struct esp_netif { bool up; esp_netif_ip_info_t ip; } esp_netif_t;
typedef struct { int type; union { esp_ip4_addr_t ip4; } u_addr; } test_ip_addr_t;
typedef struct { test_ip_addr_t ip; } esp_netif_dns_info_t;
typedef struct { uint8_t ssid[32], ssid_len, bssid[6]; } wifi_event_sta_connected_t;
typedef struct { uint8_t ssid[32], ssid_len, bssid[6], reason; } wifi_event_sta_disconnected_t;
typedef struct { esp_netif_t *esp_netif; esp_netif_ip_info_t ip_info; bool ip_changed; } ip_event_got_ip_t;
typedef struct { struct { uint32_t ip[4]; } ip6_info; } ip_event_got_ip6_t;
typedef struct { uint8_t ssid[33], bssid[6]; int rssi; uint8_t primary; } wifi_ap_record_t;
typedef struct { int nvs_enable; } wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() ((wifi_init_config_t){0})
typedef struct { struct { uint8_t ssid[32], password[64]; struct { int authmode; } threshold;
                         struct { bool capable, required; } pmf_cfg; } sta;
                 struct { uint8_t ssid[32], password[64], ssid_len, channel, max_connection;
                          int authmode; } ap; } wifi_config_t;
typedef struct { bool show_hidden; } wifi_scan_config_t;
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t esp_event_loop_create_default(void);
esp_err_t esp_event_handler_instance_register(esp_event_base_t, int32_t, esp_event_handler_t, void *, esp_event_handler_instance_t *);
esp_err_t esp_event_post(esp_event_base_t, int32_t, const void *, size_t, TickType_t);
esp_err_t esp_netif_init(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);
esp_netif_t *esp_netif_create_default_wifi_ap(void);
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *);
esp_err_t esp_netif_create_ip6_linklocal(esp_netif_t *);
esp_err_t esp_netif_set_hostname(esp_netif_t *, const char *);
bool esp_netif_is_netif_up(esp_netif_t *);
esp_err_t esp_netif_get_ip_info(esp_netif_t *, esp_netif_ip_info_t *);
esp_err_t esp_netif_set_ip_info(esp_netif_t *, const esp_netif_ip_info_t *);
esp_err_t esp_netif_str_to_ip4(const char *, esp_ip4_addr_t *);
esp_err_t esp_netif_set_dns_info(esp_netif_t *, int, const esp_netif_dns_info_t *);
esp_err_t esp_netif_dhcpc_start(esp_netif_t *);
esp_err_t esp_netif_dhcpc_stop(esp_netif_t *);
esp_err_t esp_netif_dhcps_stop(esp_netif_t *);
esp_err_t esp_netif_dhcps_start(esp_netif_t *);
esp_err_t esp_wifi_init(const wifi_init_config_t *);
esp_err_t esp_wifi_deinit(void);
typedef struct { int kind; } esp_netif_config_t;
#define ESP_NETIF_DEFAULT_WIFI_STA() {0}
#define ESP_NETIF_DEFAULT_WIFI_AP() {1}
esp_netif_t *esp_netif_new(const esp_netif_config_t *);
void esp_netif_destroy(esp_netif_t *);
esp_err_t esp_netif_attach_wifi_station(esp_netif_t *);
esp_err_t esp_netif_attach_wifi_ap(esp_netif_t *);
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void);
esp_err_t esp_wifi_clear_default_wifi_driver_and_handlers(void *);
esp_err_t esp_event_handler_instance_unregister(esp_event_base_t, int32_t, esp_event_handler_instance_t);
esp_err_t esp_wifi_set_storage(int);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t);
esp_err_t esp_wifi_get_mode(wifi_mode_t *);
esp_err_t esp_wifi_set_config(int, const wifi_config_t *);
esp_err_t esp_wifi_set_ps(int);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *, bool);
esp_err_t esp_wifi_scan_get_ap_num(uint16_t *);
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *, wifi_ap_record_t *);
esp_err_t esp_wifi_clear_ap_list(void);
#ifdef __cplusplus
}
#endif
