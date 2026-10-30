#include "read_pico_sntp.h"
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"
#include "esp_netif.h"
// This adapter executes raw lwIP APIs ONLY inside the acknowledged core call.
#define ESP_NETIF_COMPONENT_BUILD
#include "esp_sntp.h"
#undef SNTP_OPMODE_POLL
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/udp.h"
#include "sntp_stamp.h"

#if SNTP_CHECK_RESPONSE < 2 || SNTP_COMP_ROUNDTRIP != 0
#error "Read Pico SNTP needs originate/address checking and no synthetic-stamp RTT compensation"
#endif

enum { DNS_IDLE, DNS_PENDING, DNS_DONE };
enum { CORE_RESOLVE = 1, CORE_START, CORE_STOP };
static atomic_flag control = ATOMIC_FLAG_INIT;
static atomic_bool stop_requested, core_ack, accept_time;
static atomic_int dns_state, reply_result;
static atomic_uint dns_ticket;
static uint32_t ticket_counter;
static char hostname[NTP_SERVER_MAX_LEN];
static ip_addr_t address;
static esp_err_t dns_result;
static bool owned, faulted;
static bool handing_pcb;
static struct udp_pcb *reserved_pcb;
static bool (*application_clock_gate)(void);
static time_t applied_time;
static sntp_stamp_t last_stamp;
static atomic_bool stamp_failed;

struct udp_pcb *__real_udp_new_ip_type(u8_t type);
struct udp_pcb *__wrap_udp_new_ip_type(u8_t type)
{
    // Only the exact synchronous SDK init call consumes this reservation.
    // Unrelated DNS/UDP clients transparently use their original allocator.
    if (!handing_pcb) return __real_udp_new_ip_type(type);
    struct udp_pcb *pcb = reserved_pcb;
    reserved_pcb = NULL;
    return pcb;
}

static void dns_found(const char *name, const ip_addr_t *resolved, void *argument)
{
    (void)name;
    uint32_t ticket = (uint32_t)(uintptr_t)argument;
    if (ticket != atomic_load(&dns_ticket) || atomic_load(&dns_state) != DNS_PENDING) return;
    dns_result = ESP_ERR_NOT_FOUND;
    if (resolved && !ip_addr_isany(resolved) && !ip_addr_ismulticast(resolved)) {
        address = *resolved;
        dns_result = ESP_OK;
    }
    // The slot/ticket is not reused until this completion. lwIP owns a copy
    // of the name and flushes its own request after this core callback returns.
    atomic_store(&dns_state, DNS_DONE);
}

// Only this board's controlled SNTP client overrides the SDK weak apply hook.
// It is called synchronously by the SDK in TCPIP context, not a queued notify.
void sntp_sync_time(struct timeval *tv)
{
    if (!owned || !atomic_exchange(&accept_time, false)) return;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (!atomic_load(&stop_requested) && application_clock_gate && application_clock_gate()) {
        result = ESP_ERR_INVALID_RESPONSE;
        if (tv && tv->tv_sec >= 1577836800 && tv->tv_sec <= INT64_C(4102444799) &&
            tv->tv_usec >= 0 && tv->tv_usec < 1000000 && !atomic_load(&stamp_failed)) {
            result = settimeofday(tv, NULL) == 0 ? ESP_OK : ESP_FAIL;
            if (result == ESP_OK) {
                applied_time = tv->tv_sec;
                sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
            }
        }
    }
    atomic_store(&reply_result, result);
}

void __real_sntp_get_system_time(uint32_t *seconds, uint32_t *microseconds);
void __wrap_sntp_get_system_time(uint32_t *seconds, uint32_t *microseconds)
{
    __real_sntp_get_system_time(seconds, microseconds);
    if (!owned) return;
    if (!sntp_stamp_next(&last_stamp, *seconds, *microseconds, seconds, microseconds)) {
        atomic_store(&stamp_failed, true);
        atomic_store(&accept_time, false);
        atomic_store(&reply_result, ESP_ERR_INVALID_STATE);
    }
}

static esp_err_t core_operation(void *argument)
{
    unsigned operation = (unsigned)(uintptr_t)argument;
    esp_err_t result = ESP_OK;
    if (operation == CORE_RESOLVE) {
        if (atomic_load(&stop_requested)) {
            dns_result = ESP_ERR_INVALID_STATE;
            atomic_store(&dns_state, DNS_DONE);
            result = ESP_ERR_INVALID_STATE;
        } else {
            err_t resolved = dns_gethostbyname_addrtype(hostname, &address, dns_found,
                (void *)(uintptr_t)atomic_load(&dns_ticket), LWIP_DNS_ADDRTYPE_DEFAULT);
            if (resolved == ERR_OK) {
                dns_found(hostname, &address, (void *)(uintptr_t)atomic_load(&dns_ticket));
            } else if (resolved != ERR_INPROGRESS) {
                dns_result = resolved == ERR_MEM ? ESP_ERR_NO_MEM : ESP_FAIL;
                atomic_store(&dns_state, DNS_DONE);
                result = dns_result;
            }
        }
    } else if (operation == CORE_START) {
        if (atomic_load(&stop_requested) || owned || sntp_enabled()) {
            result = ESP_ERR_INVALID_STATE;
        } else {
            // Do not overwrite a disabled but configured foreign client.
            for (unsigned i = 0; i < SNTP_MAX_SERVERS; ++i) {
                if (sntp_getservername(i) || !ip_addr_isany(sntp_getserver(i))) {
                    result = ESP_ERR_INVALID_STATE;
                    break;
                }
            }
            if (result == ESP_OK) {
                // The fixed SDK asserts on a null udp_new_ip_type result.
                // Reserve before taking ownership/configuring it, so ordinary
                // allocation failure remains an error, not an application abort.
                reserved_pcb = __real_udp_new_ip_type(IPADDR_TYPE_ANY);
                if (!reserved_pcb) {
                    atomic_store(&core_ack, true);
                    return ESP_ERR_NO_MEM;
                }
                owned = true;
                sntp_setoperatingmode(SNTP_OPMODE_POLL);
                sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
                sntp_set_time_sync_notification_cb(NULL);
                // Numeric address only: SDK SNTP never owns a DNS callback or
                // borrows the live configuration/request string on this path.
                sntp_setservername(0, NULL);
                sntp_setserver(0, &address);
                atomic_store(&accept_time, true);
                handing_pcb = true;
                sntp_init();
                handing_pcb = false;
                if (reserved_pcb) {
                    // ABI/SDK mismatch: reservation was not consumed. Clean
                    // only this exact PCB and refuse to report startup success.
                    udp_remove(reserved_pcb);
                    reserved_pcb = NULL;
                    result = ESP_ERR_INVALID_RESPONSE;
                }
                if (!sntp_enabled()) result = ESP_FAIL;
            }
        }
    } else if (operation == CORE_STOP) {
        atomic_store(&accept_time, false);
        if (owned) {
            sntp_stop();
            if (sntp_enabled()) {
                result = ESP_FAIL;
            } else {
                ip_addr_t empty;
                ip_addr_set_zero(&empty);
                for (unsigned i = 0; i < SNTP_MAX_SERVERS; ++i) {
                    sntp_setservername(i, NULL);
                    sntp_setserver(i, &empty);
                }
                sntp_set_time_sync_notification_cb(NULL);
                owned = false;
            }
        }
    } else {
        result = ESP_ERR_INVALID_ARG;
    }
    atomic_store(&core_ack, true);
    return result;
}

static esp_err_t call_core(unsigned operation)
{
    atomic_store(&core_ack, false);
    esp_err_t result = esp_netif_tcpip_exec(core_operation, (void *)(uintptr_t)operation);
    // Require callback execution acknowledgement as well as IPC return. Do
    // not infer ownership/exit from a nominal successful transport result.
    if (!atomic_load(&core_ack)) {
        faulted = true;
        return result == ESP_OK ? ESP_ERR_INVALID_RESPONSE : result;
    }
    return result;
}

static esp_err_t close_owned_service(void)
{
    // Suppress late clock application even when the stop command itself never
    // executes/acknowledges. This does not claim the SDK service has exited.
    atomic_store(&accept_time, false);
    if (!owned) return ESP_OK;
    esp_err_t result = call_core(CORE_STOP);
    if (result != ESP_OK) faulted = true;
    if (result == ESP_OK) application_clock_gate = NULL;
    return result;
}

void read_pico_sntp_request_stop(void)
{
    atomic_store(&stop_requested, true);
    atomic_store(&accept_time, false);
}

esp_err_t read_pico_sntp_stop(void)
{
    read_pico_sntp_request_stop();
    if (atomic_flag_test_and_set(&control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = close_owned_service();
    if (result == ESP_OK && atomic_load(&dns_state) == DNS_PENDING) result = ESP_ERR_NOT_FINISHED;
    if (result == ESP_OK && atomic_load(&dns_state) == DNS_DONE) {
        // DNS completion publishes before lwIP finishes flushing its request.
        // Fence the core before allowing radio teardown, without clearing the
        // global DNS cache or stopping another client's SNTP service.
        result = call_core(CORE_STOP);
        if (result == ESP_OK) atomic_store(&dns_state, DNS_IDLE);
        else faulted = true;
    }
    // Missing core ACK is not cleared by init/retry or by pretending no owned
    // handle means successful exit. Keep uncertain state until explicit audit.
    if (result == ESP_OK && faulted) result = ESP_ERR_INVALID_STATE;
    atomic_flag_clear(&control);
    return result;
}

esp_err_t read_pico_sntp_sync(const char *server, bool (*allowed)(void),
                             bool (*clock_gate)(void), time_t *confirmed)
{
    if (!server || !allowed || !clock_gate || !confirmed) return ESP_ERR_INVALID_ARG;
    size_t length = strnlen(server, sizeof(hostname));
    if (!length || length == sizeof(hostname)) return ESP_ERR_INVALID_SIZE;
    for (size_t i = 0; i < length; ++i)
        if ((unsigned char)server[i] <= 0x20 || (unsigned char)server[i] > 0x7e) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (atomic_load(&stop_requested) || faulted || owned || !allowed()) goto done;
    if (atomic_load(&dns_state) == DNS_PENDING) {
        result = ESP_ERR_NOT_FINISHED;
        goto done;
    }
    if (ticket_counter == UINT32_MAX) goto done;
    memcpy(hostname, server, length + 1);
    atomic_store(&dns_ticket, ++ticket_counter);
    atomic_store(&dns_state, DNS_PENDING);
    atomic_store(&reply_result, ESP_ERR_NOT_FINISHED);
    atomic_store(&stamp_failed, false);
    TickType_t started = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS(10000);
    TickType_t pause = pdMS_TO_TICKS(100);
    if (!pause) pause = 1;
    result = call_core(CORE_RESOLVE);
    if (result != ESP_OK) goto done;
    while (atomic_load(&dns_state) == DNS_PENDING) {
        if (atomic_load(&stop_requested) || !allowed()) {
            result = ESP_ERR_INVALID_STATE;
            goto done;
        }
        if ((TickType_t)(xTaskGetTickCount() - started) >= limit) {
            result = ESP_ERR_TIMEOUT;
            goto done;
        }
        vTaskDelay(pause);
    }
    result = dns_result;
    if (result != ESP_OK) goto done;
    if (atomic_load(&stop_requested) || !allowed()) {
        result = ESP_ERR_INVALID_STATE;
        goto done;
    }
    if ((TickType_t)(xTaskGetTickCount() - started) >= limit) {
        result = ESP_ERR_TIMEOUT;
        goto done;
    }
    application_clock_gate = clock_gate;
    result = call_core(CORE_START);
    if (result != ESP_OK) {
        if (owned) faulted = true;
        goto cleanup;
    }
    while (atomic_load(&reply_result) == ESP_ERR_NOT_FINISHED) {
        if (atomic_load(&stop_requested) || !allowed()) {
            result = ESP_ERR_INVALID_STATE;
            goto cleanup;
        }
        if ((TickType_t)(xTaskGetTickCount() - started) >= limit) {
            result = ESP_ERR_TIMEOUT;
            goto cleanup;
        }
        vTaskDelay(pause);
    }
    result = atomic_load(&reply_result);
    if (atomic_load(&stamp_failed)) faulted = true;
cleanup: {
        esp_err_t closed = close_owned_service();
        if (closed != ESP_OK) result = closed;
    }
    if (result == ESP_OK && (atomic_load(&stop_requested) || !allowed())) result = ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) *confirmed = applied_time;
done:
    atomic_flag_clear(&control);
    return result;
}
