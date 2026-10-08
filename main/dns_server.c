#include "dns_server.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#if CONFIG_BOARD_DRIVER_READ_PICO
#include "captive_dns.h"
#include "freertos/event_groups.h"
#include <stdatomic.h>
#include <errno.h>

enum { DNS_STOPPED, DNS_RUNNING, DNS_STOPPING, DNS_FAILED };
static atomic_int s_state = DNS_STOPPED;
static atomic_bool s_stop, s_close_ok;
static atomic_flag s_control = ATOMIC_FLAG_INIT;
static EventGroupHandle_t s_done;
static atomic_int s_worker_socket = -1;
#define DNS_DONE_BIT BIT0
static const char *TAG = "dns_server";

static void close_owned_socket(int fd)
{
    s_close_ok = close(fd) == 0;
    if (s_close_ok) s_worker_socket = -1;
    else ESP_LOGE(TAG, "DNS socket close unconfirmed; no reuse");
}

static void dns_server_task(void *unused)
{
    (void)unused;
    int fd = s_worker_socket;
    uint8_t query[CAPTIVE_DNS_MAX_PACKET + 1], response[CAPTIVE_DNS_MAX_PACKET];
    const uint8_t address[4] = {192, 168, 4, 1};
    bool failed = false;
    while (!s_stop) {
        struct sockaddr_in client = {0};
        socklen_t client_length = sizeof(client);
        int length = recvfrom(fd, query, sizeof(query), 0, (struct sockaddr *)&client, &client_length);
        if (length < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                vTaskDelay(pdMS_TO_TICKS(1)); continue;
            }
            failed = true; break;
        }
        if (s_stop) break;
        size_t written;
        if (client_length == sizeof(client) && client.sin_family == AF_INET &&
            captive_dns_response(query, (size_t)length, address, response, sizeof(response), &written)) {
            int sent = sendto(fd, response, written, 0, (struct sockaddr *)&client, client_length);
            if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                vTaskDelay(pdMS_TO_TICKS(1)); continue;
            }
            if (sent != (int)written) { failed = true; break; }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    close_owned_socket(fd); // Worker is the only socket owner after task creation.
    if (failed) ESP_LOGE(TAG, "DNS worker failed; explicit stop acknowledgement required");
    s_state = DNS_FAILED; // Not restartable until stop_checked acknowledges completion.
    xEventGroupSetBits(s_done, DNS_DONE_BIT);
    // No shared-state access after acknowledgement; a new start cannot affect this exit.
    vTaskDelete(NULL);
}

esp_err_t dns_server_start(void)
{
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (s_state == DNS_RUNNING) { result = ESP_OK; goto finished; }
    if (s_state != DNS_STOPPED) goto finished;
    if (!s_done) s_done = xEventGroupCreate();
    if (!s_done) { result = ESP_ERR_NO_MEM; goto finished; }
    xEventGroupClearBits(s_done, DNS_DONE_BIT);
    s_stop = false; s_close_ok = false;
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) { result = ESP_FAIL; goto finished; }
    s_worker_socket = fd;
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 250000};
    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(0xc0a80401); // Only the provisioning AP, not station/ANY.
    local.sin_port = htons(53);
    result = ESP_FAIL;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0 ||
        bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0) goto failed;
    s_state = DNS_RUNNING;
    if (xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5, NULL) != pdPASS) goto failed;
    result = s_state == DNS_RUNNING ? ESP_OK : ESP_FAIL;
    goto finished;
failed:
    // No task was created: ownership is still with this caller.
    close_owned_socket(fd);
    s_state = s_close_ok ? DNS_STOPPED : DNS_FAILED;
    xEventGroupSetBits(s_done, DNS_DONE_BIT);
finished:
    atomic_flag_clear(&s_control);
    return result;
}

esp_err_t dns_server_stop_checked(void)
{
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_OK;
    if (s_state != DNS_STOPPED) {
        s_stop = true;
        s_state = DNS_STOPPING;
        EventBits_t done = xEventGroupWaitBits(s_done, DNS_DONE_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(2000));
        if (!(done & DNS_DONE_BIT)) result = ESP_ERR_TIMEOUT;
        else if (!s_close_ok) { result = ESP_FAIL; s_state = DNS_FAILED; }
        else s_state = DNS_STOPPED;
    }
    atomic_flag_clear(&s_control);
    return result;
}

void dns_server_stop(void)
{
    esp_err_t stopped = dns_server_stop_checked();
    if (stopped != ESP_OK) ESP_LOGE(TAG, "DNS stop unconfirmed: %s", esp_err_to_name(stopped));
}
#else

static const char *TAG = "dns_server";
static int dns_socket = -1;
static TaskHandle_t dns_task_handle = NULL;
static bool dns_running = false;

#define DNS_PORT 53
#define DNS_MAX_LEN 512

// DNS header structure
typedef struct __attribute__((packed)) {
    uint16_t id;
    uint16_t flags;
    uint16_t questions;
    uint16_t answers;
    uint16_t authority;
    uint16_t additional;
} dns_header_t;

// DNS answer structure
typedef struct __attribute__((packed)) {
    uint16_t name_ptr;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t data_len;
    uint32_t ip_addr;
} dns_answer_t;

static void dns_server_task(void *pvParameters)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    uint8_t rx_buffer[DNS_MAX_LEN];
    uint8_t tx_buffer[DNS_MAX_LEN];

    // Create UDP socket
    dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (dns_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        vTaskDelete(NULL);
        return;
    }

    // Bind to DNS port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(DNS_PORT);

    if (bind(dns_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(dns_socket);
        dns_socket = -1;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "DNS server started on port %d", DNS_PORT);
    dns_running = true;

    while (dns_running) {
        int len = recvfrom(dns_socket, rx_buffer, sizeof(rx_buffer), 0,
                           (struct sockaddr *) &client_addr, &client_addr_len);

        if (len < sizeof(dns_header_t)) {
            continue;
        }

        dns_header_t *header = (dns_header_t *) rx_buffer;

        // Only respond to queries
        if ((ntohs(header->flags) & 0x8000) == 0) {
            // Build response
            memcpy(tx_buffer, rx_buffer, len);
            dns_header_t *response_header = (dns_header_t *) tx_buffer;

            // Set response flags
            response_header->flags = htons(0x8180);  // Standard query response, no error
            response_header->answers = htons(1);
            response_header->authority = 0;
            response_header->additional = 0;

            // Add answer pointing to 192.168.4.1
            dns_answer_t *answer = (dns_answer_t *) (tx_buffer + len);
            answer->name_ptr = htons(0xC00C);     // Pointer to domain name in question
            answer->type = htons(1);              // A record
            answer->class = htons(1);             // IN
            answer->ttl = htonl(60);              // 60 seconds TTL
            answer->data_len = htons(4);          // IPv4 address length
            answer->ip_addr = htonl(0xC0A80401);  // 192.168.4.1

            int response_len = len + sizeof(dns_answer_t);

            sendto(dns_socket, tx_buffer, response_len, 0, (struct sockaddr *) &client_addr,
                   client_addr_len);
        }
    }

    close(dns_socket);
    dns_socket = -1;
    vTaskDelete(NULL);
}

esp_err_t dns_server_start(void)
{
    if (dns_task_handle != NULL) {
        ESP_LOGW(TAG, "DNS server already running");
        return ESP_OK;
    }

    BaseType_t ret = xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5, &dns_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create DNS server task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void dns_server_stop(void)
{
    if (dns_task_handle != NULL) {
        dns_running = false;
        if (dns_socket >= 0) {
            close(dns_socket);
            dns_socket = -1;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        dns_task_handle = NULL;
        ESP_LOGI(TAG, "DNS server stopped");
    }
}

esp_err_t dns_server_stop_checked(void) { dns_server_stop(); return ESP_OK; }
#endif
