#include "read_pico_ota.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs.h"
#include "ota_local.h"
#include "read_pico_board.h"

#define OTA_NVS_NAMESPACE "rp_ota"
#define OTA_NVS_PENDING "pending"
#define OTA_NVS_TRIES "tries"
#define OTA_NVS_PREVIOUS "previous"
#define OTA_RECV_CHUNK 4096
#define OTA_RECV_TIMEOUT_RETRIES 8
#define OTA_RESTART_DELAY_US (2 * 1000 * 1000)
// Longer than one photo refresh; see read_pico_sd.h.
#define OTA_BOARD_LOCK_WAIT_MS 5000
// A trial boot that never reaches "healthy" (early return, hang) restarts so
// the next boot is counted; after the budget the guard falls back.
#define OTA_TRIAL_DEADLINE_US (5LL * 60 * 1000 * 1000)

static const char *TAG = "rp_ota";
static atomic_bool s_busy;
static bool s_pending_seen;
static esp_timer_handle_t s_trial_timer;

static bool running_from_ota_slot(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    return running && running->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
           running->subtype <= ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
}

static bool supported(void)
{
    return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL) &&
           esp_ota_get_next_update_partition(NULL);
}

static const esp_partition_t *previous_partition(nvs_handle_t nvs)
{
    char label[17] = "";
    size_t size = sizeof(label);
    const esp_partition_t *found = NULL;
    if (nvs_get_str(nvs, OTA_NVS_PREVIOUS, label, &size) == ESP_OK && label[0])
        found = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if (!found || found == esp_ota_get_running_partition())
        found = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    return found;
}

static void restart_cb(void *arg)
{
    (void) arg;
    esp_restart();
}

void read_pico_ota_boot_guard(void)
{
    nvs_handle_t nvs;
    if (nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
        return;  // nothing pending was ever written
    uint8_t pending = 0, tries = 0;
    (void) nvs_get_u8(nvs, OTA_NVS_PENDING, &pending);
    (void) nvs_get_u8(nvs, OTA_NVS_TRIES, &tries);
    switch (ota_local_boot_action(running_from_ota_slot(), pending, tries)) {
    case OTA_BOOT_NORMAL:
        break;
    case OTA_BOOT_TRIAL:
        s_pending_seen = true;
        if (nvs_set_u8(nvs, OTA_NVS_TRIES, (uint8_t) (tries + 1)) == ESP_OK)
            nvs_commit(nvs);
        ESP_LOGW(TAG, "Trial boot %u/%u of a new image", tries + 1, OTA_LOCAL_MAX_BOOT_TRIES);
        {
            const esp_timer_create_args_t args = {.callback = restart_cb, .name = "ota_trial"};
            if (esp_timer_create(&args, &s_trial_timer) != ESP_OK ||
                esp_timer_start_once(s_trial_timer, OTA_TRIAL_DEADLINE_US) != ESP_OK)
                ESP_LOGE(TAG, "Trial deadline not armed; a hung image will not be retried");
        }
        break;
    case OTA_BOOT_FALL_BACK: {
        const esp_partition_t *previous = previous_partition(nvs);
        nvs_set_u8(nvs, OTA_NVS_PENDING, 0);
        nvs_set_u8(nvs, OTA_NVS_TRIES, 0);
        nvs_commit(nvs);
        nvs_close(nvs);
        if (previous && esp_ota_set_boot_partition(previous) == ESP_OK) {
            ESP_LOGE(TAG, "New image never confirmed healthy; returning to %s", previous->label);
            esp_restart();
        }
        ESP_LOGE(TAG, "New image unconfirmed and no previous image to return to");
        return;
    }
    }
    nvs_close(nvs);
}

void read_pico_ota_mark_healthy(void)
{
    // Harmless when the bootloader has no rollback support (state unknown).
    (void) esp_ota_mark_app_valid_cancel_rollback();
    if (!s_pending_seen)
        return;
    if (s_trial_timer) {
        esp_timer_stop(s_trial_timer);
        esp_timer_delete(s_trial_timer);
        s_trial_timer = NULL;
    }
    nvs_handle_t nvs;
    if (nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
        return;
    if (nvs_set_u8(nvs, OTA_NVS_PENDING, 0) == ESP_OK && nvs_set_u8(nvs, OTA_NVS_TRIES, 0) == ESP_OK &&
        nvs_commit(nvs) == ESP_OK) {
        s_pending_seen = false;
        ESP_LOGI(TAG, "New image confirmed healthy");
    }
    nvs_close(nvs);
}

static esp_err_t reply(httpd_req_t *req, const char *status, const char *message, const char *version)
{
    cJSON *json = cJSON_CreateObject();
    if (json) {
        cJSON_AddStringToObject(json, "status", status);
        cJSON_AddStringToObject(json, "message", message);
        if (version) cJSON_AddStringToObject(json, "version", version);
    }
    char *text = json ? cJSON_PrintUnformatted(json) : NULL;
    cJSON_Delete(json);
    if (strcmp(status, "success")) httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, text ? text : "{}");
    free(text);
    return err;
}

// Read exactly n bytes (or the rest of the body) with bounded socket timeouts.
static int receive(httpd_req_t *req, uint8_t *buffer, size_t n)
{
    size_t got = 0;
    unsigned timeouts = 0;
    while (got < n) {
        int r = httpd_req_recv(req, (char *) buffer + got, n - got);
        if (r == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < OTA_RECV_TIMEOUT_RETRIES) continue;
        if (r <= 0) return -1;
        timeouts = 0;
        got += (size_t) r;
    }
    return (int) got;
}

static esp_err_t receive_locked(httpd_req_t *req)
{
    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (!supported() || !target)
        return reply(req, "error", "OTA partition table not installed on this device", NULL);
    size_t total = req->content_len;
    uint8_t *buffer = malloc(OTA_RECV_CHUNK);
    if (!buffer) return reply(req, "error", "Out of memory", NULL);
    char version[32] = "";
    int first = total >= OTA_LOCAL_HEADER_BYTES ? receive(req, buffer, MIN(total, OTA_RECV_CHUNK)) : -1;
    ota_image_verdict_t verdict = first < 0 ? OTA_IMAGE_TOO_SHORT
        : ota_local_check_image(buffer, (size_t) first, total, target->size, version);
    if (verdict != OTA_IMAGE_OK) {
        free(buffer);
        ESP_LOGW(TAG, "Upload refused: %s", ota_local_verdict_text(verdict));
        return reply(req, "error", ota_local_verdict_text(verdict), NULL);
    }
    // Flash erase/write stalls the cache; never overlap a panel refresh. While
    // held, refreshes fail as busy and SD/touch users wait or time out.
    if (read_pico_board_lock(OTA_BOARD_LOCK_WAIT_MS) != ESP_OK) {
        free(buffer);
        return reply(req, "error", "Display busy; try again", NULL);
    }
    esp_ota_handle_t handle = 0;
    esp_err_t err = esp_ota_begin(target, total, &handle);
    size_t written = 0;
    if (err == ESP_OK) {
        err = esp_ota_write(handle, buffer, (size_t) first);
        written = (size_t) first;
    }
    while (err == ESP_OK && written < total) {
        int r = receive(req, buffer, MIN(total - written, OTA_RECV_CHUNK));
        if (r < 0) { err = ESP_ERR_INVALID_SIZE; break; }
        err = esp_ota_write(handle, buffer, (size_t) r);
        written += (size_t) r;
    }
    free(buffer);
    if (err == ESP_OK) err = esp_ota_end(handle);  // verifies image checksum/hash
    else if (handle) esp_ota_abort(handle);
    read_pico_board_unlock();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Writing %s failed: %s", target->label, esp_err_to_name(err));
        return reply(req, "error", err == ESP_ERR_OTA_VALIDATE_FAILED ? "Image verification failed"
                                                                      : "Firmware write failed",
                     NULL);
    }
    nvs_handle_t nvs;
    err = nvs_open(OTA_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        const esp_partition_t *running = esp_ota_get_running_partition();
        err = nvs_set_str(nvs, OTA_NVS_PREVIOUS, running ? running->label : "");
        if (err == ESP_OK) err = nvs_set_u8(nvs, OTA_NVS_PENDING, 1);
        if (err == ESP_OK) err = nvs_set_u8(nvs, OTA_NVS_TRIES, 0);
        if (err == ESP_OK) err = nvs_commit(nvs);
        nvs_close(nvs);
    }
    // Without the pending record the guard could not return to this image.
    if (err == ESP_OK) err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not select %s: %s", target->label, esp_err_to_name(err));
        return reply(req, "error", "Could not select the new image; still running the current one", NULL);
    }
    ESP_LOGI(TAG, "Wrote %u bytes to %s (%s); restarting", (unsigned) written, target->label, version);
    reply(req, "success", "Firmware installed; restarting", version);
    const esp_timer_create_args_t args = {.callback = restart_cb, .name = "ota_restart"};
    esp_timer_handle_t timer;
    if (esp_timer_create(&args, &timer) != ESP_OK || esp_timer_start_once(timer, OTA_RESTART_DELAY_US) != ESP_OK)
        esp_restart();
    return ESP_OK;
}

esp_err_t read_pico_ota_receive(httpd_req_t *req)
{
    if (atomic_exchange(&s_busy, true))
        return reply(req, "error", "Another firmware upload is in progress", NULL);
    esp_err_t err = receive_locked(req);
    atomic_store(&s_busy, false);
    return err;
}

void read_pico_ota_describe(cJSON *object)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    bool ok = supported();
    cJSON_AddBoolToObject(object, "supported", ok);
    cJSON_AddStringToObject(object, "running", running ? running->label : "");
    cJSON_AddStringToObject(object, "next", ok && next ? next->label : "");
    cJSON_AddNumberToObject(object, "slot_size", ok && next ? (double) next->size : 0);
    cJSON_AddBoolToObject(object, "pending", s_pending_seen);
    cJSON_AddStringToObject(object, "version", esp_app_get_description()->version);
}
