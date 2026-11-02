#include "read_pico_hotspot.h"

#include <stdatomic.h>
#include <string.h>

#include "config.h"
#include "config_manager.h"
#include "display_manager.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "hotspot_policy.h"
#include "nvs.h"
#include "power_manager.h"
#include "provisioning_session.h"
#include "utils.h"
#include "wifi_credentials.h"
#include "wifi_manager.h"
#include "wifi_provisioning_ap.h"

#define HOTSPOT_RECONNECT_FIRST_MS 30000
#define HOTSPOT_RECONNECT_MAX_MS 600000
#define HOTSPOT_RANDOM_BYTES 64
// Let the save response reach the phone before restarting (as the old portal did).
#define HOTSPOT_SAVE_RESTART_MS 3000
// A middle-key press read right after a redraw is the same press: ignore it.
#define HOTSPOT_SCREEN_DEBOUNCE_MS 1500

static const char *TAG = "hotspot";

// Shared with request/configure/describe callers; never held across a blocking call.
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static hotspot_policy_t s_policy;
static char s_ssid[HOTSPOT_SSID_MAX + 1];  // custom name, "" = device default
static char s_password[HOTSPOT_PASSWORD_MAX + 1];
static uint32_t s_idle_minutes = HOTSPOT_IDLE_DEFAULT_MIN;
static char s_active_ssid[HOTSPOT_SSID_MAX + 1], s_active_password[HOTSPOT_PASSWORD_MAX + 1];
static esp_err_t s_last_error = ESP_OK;
static atomic_bool s_ready, s_show_requested;
static atomic_uint s_clients;
static atomic_int s_transfers;
static atomic_int s_mode = READ_PICO_HOTSPOT_MANUAL;

// Owner task (read_pico_hotspot_poll) only.
static bool s_screen_shown;
static char s_restore_path[DISPLAY_IMAGE_PATH_CAPACITY];
static bool s_reconnect_pending;
static int64_t s_reconnect_at_ms;
static uint32_t s_reconnect_backoff_ms;
static hotspot_retry_t s_retry;
static int64_t s_restart_at_ms;  // 0 = none scheduled
static int64_t s_screen_done_ms;

// The sleep variant may run on battery, where a hotspot must always close by itself.
static bool battery_possible(void)
{
#if CONFIG_READ_PICO_PMU_SLEEP
    return true;
#else
    return false;
#endif
}

static int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

// Copies at most capacity - 1 bytes and always terminates (zero-filled tail).
static void copy_bounded(char *out, size_t capacity, const char *text)
{
    size_t length = strnlen(text, capacity - 1);
    memset(out, 0, capacity);
    memcpy(out, text, length);
}

// get_setup_ap_ssid() is built once in init, so this only reads static buffers.
static void copy_effective_ssid_locked(char *out)
{
    copy_bounded(out, HOTSPOT_SSID_MAX + 1, s_ssid[0] ? s_ssid : get_setup_ap_ssid());
}

static esp_err_t load_string(nvs_handle_t handle, const char *key, char *out, size_t capacity)
{
    size_t length = capacity;
    char value[HOTSPOT_PASSWORD_MAX + 1];
    if (capacity > sizeof(value)) return ESP_ERR_INVALID_SIZE;
    esp_err_t err = nvs_get_str(handle, key, value, &length);
    if (err == ESP_OK) memcpy(out, value, length);
    return err;
}

static bool generate_password(char *out)
{
    uint8_t random[HOTSPOT_RANDOM_BYTES];
    esp_fill_random(random, sizeof(random));
    bool ok = hotspot_password_generate(random, sizeof(random), out);
    memset(random, 0, sizeof(random));
    return ok;
}

// Writes only the given keys (NULL = skip) with one commit; RAM changes only after it.
static esp_err_t save_settings(const char *ssid, const char *password, int idle_minutes)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    if (ssid) err = nvs_set_str(handle, NVS_HOTSPOT_SSID_KEY, ssid);
    if (err == ESP_OK && password) err = nvs_set_str(handle, NVS_HOTSPOT_PASS_KEY, password);
    if (err == ESP_OK && idle_minutes >= 0)
        err = nvs_set_u8(handle, NVS_HOTSPOT_IDLE_KEY, (uint8_t) idle_minutes);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t read_pico_hotspot_init(void)
{
    if (s_ready) return ESP_OK;
    (void) get_setup_ap_ssid();  // reads the MAC once, outside any critical section
    char ssid[sizeof(s_ssid)] = "", password[sizeof(s_password)] = "";
    uint8_t idle = HOTSPOT_IDLE_DEFAULT_MIN;
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err == ESP_OK) {
        esp_err_t got = load_string(handle, NVS_HOTSPOT_SSID_KEY, ssid, sizeof(ssid));
        if (got != ESP_OK || !hotspot_ssid_valid(ssid)) ssid[0] = 0;
        got = load_string(handle, NVS_HOTSPOT_PASS_KEY, password, sizeof(password));
        if (got != ESP_OK || !hotspot_password_valid(password)) password[0] = 0;
        if (nvs_get_u8(handle, NVS_HOTSPOT_IDLE_KEY, &idle) != ESP_OK ||
            !hotspot_idle_minutes_valid(idle, battery_possible()))
            idle = HOTSPOT_IDLE_DEFAULT_MIN;
        nvs_close(handle);
    } else if (err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Settings unreadable (%s); hotspot disabled", esp_err_to_name(err));
        return err;
    }
    if (!password[0]) {
        // First use: one random password, kept so phones remember the network.
        if (!generate_password(password)) return ESP_FAIL;
        err = save_settings(NULL, password, -1);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Password save failed (%s); hotspot disabled", esp_err_to_name(err));
            return err;
        }
    }
    taskENTER_CRITICAL(&s_lock);
    memcpy(s_ssid, ssid, sizeof(s_ssid));
    memcpy(s_password, password, sizeof(s_password));
    s_idle_minutes = idle;
    hotspot_policy_init(&s_policy, idle);
    taskEXIT_CRITICAL(&s_lock);
    s_ready = true;
    ESP_LOGI(TAG, "Ready (off); auto-close %u min", (unsigned) idle);
    return ESP_OK;
}

esp_err_t read_pico_hotspot_request(bool on)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    taskENTER_CRITICAL(&s_lock);
    bool accepted = hotspot_policy_request(&s_policy, on, now_ms());
    taskEXIT_CRITICAL(&s_lock);
    if (accepted) ESP_LOGI(TAG, "Requested %s", on ? "on" : "off");
    return accepted ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t read_pico_hotspot_toggle(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    taskENTER_CRITICAL(&s_lock);
    bool on = !hotspot_policy_wants_on(&s_policy);
    taskEXIT_CRITICAL(&s_lock);
    return read_pico_hotspot_request(on);
}

esp_err_t read_pico_hotspot_start_fallback(read_pico_hotspot_mode_t mode)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (mode != READ_PICO_HOTSPOT_SETUP && mode != READ_PICO_HOTSPOT_RECOVERY) return ESP_ERR_INVALID_ARG;
    int64_t now = now_ms();
    taskENTER_CRITICAL(&s_lock);
    s_mode = mode;
    // Without home Wi-Fi this is the only way in: it stays until set up, except on battery.
    hotspot_policy_set_idle_minutes(&s_policy, battery_possible() ? s_idle_minutes : 0, now);
    bool accepted = hotspot_policy_request(&s_policy, true, now);
    taskEXIT_CRITICAL(&s_lock);
    ESP_LOGW(TAG, "%s: opening the device hotspot", mode == READ_PICO_HOTSPOT_SETUP
                                                       ? "No saved Wi-Fi" : "Saved Wi-Fi unreachable");
    return accepted ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t read_pico_hotspot_show_screen(void)
{
    if (!read_pico_hotspot_on()) return ESP_ERR_INVALID_STATE;
    s_show_requested = true;
    return ESP_OK;
}

bool read_pico_hotspot_owns_radio(void)
{
    if (!s_ready) return false;
    taskENTER_CRITICAL(&s_lock);
    bool owns = hotspot_policy_owns_radio(&s_policy);
    taskEXIT_CRITICAL(&s_lock);
    return owns;
}

bool read_pico_hotspot_on(void)
{
    if (!s_ready) return false;
    taskENTER_CRITICAL(&s_lock);
    bool on = s_policy.state == HOTSPOT_ON;
    taskEXIT_CRITICAL(&s_lock);
    return on;
}

void read_pico_hotspot_transfer(bool begin)
{
    if (begin) atomic_fetch_add(&s_transfers, 1);
    else atomic_fetch_sub(&s_transfers, 1);
}

esp_err_t read_pico_hotspot_configure(const read_pico_hotspot_change_t *change)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (!change) return ESP_ERR_INVALID_ARG;
    if (change->ssid && change->ssid[0] && !hotspot_ssid_valid(change->ssid)) return ESP_ERR_INVALID_ARG;
    if (change->password && !hotspot_password_valid(change->password)) return ESP_ERR_INVALID_ARG;
    if (change->idle_minutes >= 0 &&
        !hotspot_idle_minutes_valid((uint32_t) change->idle_minutes, battery_possible()))
        return ESP_ERR_INVALID_ARG;
    char password[sizeof(s_password)];
    const char *new_password = change->password;
    if (!new_password && change->regenerate_password) {
        if (!generate_password(password)) return ESP_FAIL;
        new_password = password;
    }
    esp_err_t err = save_settings(change->ssid, new_password, change->idle_minutes);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Settings save failed (%s); keeping the previous values", esp_err_to_name(err));
        return err;
    }
    taskENTER_CRITICAL(&s_lock);
    if (change->ssid) copy_bounded(s_ssid, sizeof(s_ssid), change->ssid);
    if (new_password) copy_bounded(s_password, sizeof(s_password), new_password);
    if (change->idle_minutes >= 0) {
        s_idle_minutes = (uint32_t) change->idle_minutes;
        hotspot_policy_set_idle_minutes(&s_policy, s_idle_minutes, now_ms());
    }
    taskEXIT_CRITICAL(&s_lock);
    memset(password, 0, sizeof(password));
    return ESP_OK;
}

esp_err_t read_pico_hotspot_describe(cJSON *object)
{
    if (!object) return ESP_ERR_INVALID_ARG;
    if (!s_ready) return cJSON_AddBoolToObject(object, "supported", false) ? ESP_OK : ESP_ERR_NO_MEM;
    char ssid[sizeof(s_ssid)], password[sizeof(s_password)];
    char active_ssid[sizeof(s_ssid)], active_password[sizeof(s_password)];
    taskENTER_CRITICAL(&s_lock);
    hotspot_policy_t policy = s_policy;
    copy_effective_ssid_locked(ssid);
    memcpy(password, s_password, sizeof(password));
    memcpy(active_ssid, s_active_ssid, sizeof(active_ssid));
    memcpy(active_password, s_active_password, sizeof(active_password));
    bool custom = s_ssid[0] != 0;
    uint32_t idle = policy.idle_limit_ms / 60000;
    esp_err_t last_error = s_last_error;
    taskEXIT_CRITICAL(&s_lock);
    bool on = policy.state == HOTSPOT_ON;
    // While on, show what phones are actually joining; edits apply next start.
    bool pending_change = on && (strcmp(ssid, active_ssid) || strcmp(password, active_password));
    int64_t remaining = hotspot_policy_remaining_ms(&policy, now_ms());
    int mode = atomic_load(&s_mode);
    char home_ssid[WIFI_SSID_MAX_LEN] = {0}, home_password[WIFI_PASS_MAX_LEN] = {0};
    bool home_configured = wifi_manager_load_credentials(home_ssid, home_password) == ESP_OK;
    memset(home_password, 0, sizeof(home_password));
    if (!home_configured) home_ssid[0] = 0;
    // Owner-task fields read without a lock: a stale value only skews a display.
    int64_t retry = mode == READ_PICO_HOTSPOT_RECOVERY ? hotspot_retry_remaining_ms(&s_retry, now_ms()) : -1;
    bool ok = cJSON_AddBoolToObject(object, "supported", true) &&
              cJSON_AddStringToObject(object, "state", hotspot_state_name(policy.state)) &&
              cJSON_AddBoolToObject(object, "on", on) &&
              cJSON_AddBoolToObject(object, "wants_on", hotspot_policy_wants_on(&policy)) &&
              cJSON_AddStringToObject(object, "ssid", on ? active_ssid : ssid) &&
              cJSON_AddStringToObject(object, "password", on ? active_password : password) &&
              cJSON_AddStringToObject(object, "default_ssid", get_setup_ap_ssid()) &&
              cJSON_AddBoolToObject(object, "custom_ssid", custom) &&
              cJSON_AddBoolToObject(object, "pending_change", pending_change) &&
              cJSON_AddStringToObject(object, "url", READ_PICO_HOTSPOT_URL) &&
              cJSON_AddNumberToObject(object, "idle_minutes", idle) &&
              cJSON_AddBoolToObject(object, "never_close_allowed", !battery_possible()) &&
              cJSON_AddNumberToObject(object, "clients", atomic_load(&s_clients)) &&
              cJSON_AddNumberToObject(object, "transfers", atomic_load(&s_transfers)) &&
              cJSON_AddNumberToObject(object, "remaining_s",
                                      remaining < 0 ? -1 : (double) ((remaining + 999) / 1000)) &&
              cJSON_AddStringToObject(object, "last_stop_reason",
                                      hotspot_reason_name(policy.last_stop_reason)) &&
              cJSON_AddBoolToObject(object, "stop_unconfirmed", policy.stop_unconfirmed) &&
              cJSON_AddStringToObject(object, "last_error", esp_err_to_name(last_error)) &&
              cJSON_AddStringToObject(object, "mode", mode == READ_PICO_HOTSPOT_SETUP ? "setup"
                                                      : mode == READ_PICO_HOTSPOT_RECOVERY ? "recovery"
                                                                                            : "manual") &&
              cJSON_AddBoolToObject(object, "home_configured", home_configured) &&
              cJSON_AddStringToObject(object, "home_ssid", home_ssid) &&
              cJSON_AddNumberToObject(object, "retry_in_s",
                                      retry < 0 ? -1 : (double) ((retry + 999) / 1000)) &&
              cJSON_AddBoolToObject(object, "restarting", s_restart_at_ms != 0);
    memset(password, 0, sizeof(password));
    memset(active_password, 0, sizeof(active_password));
    return ok ? ESP_OK : ESP_ERR_NO_MEM;
}

static void show_screen(void)
{
    char current[DISPLAY_IMAGE_PATH_CAPACITY];
    bool known = false;
    if (display_manager_copy_current_image(current, sizeof(current), &known) &&
        (current[0] || !s_screen_shown))
        memcpy(s_restore_path, current, sizeof(s_restore_path));  // what to bring back
    char qr[2 * (HOTSPOT_SSID_MAX + HOTSPOT_PASSWORD_MAX) + 32];
    if (!hotspot_wifi_qr(s_active_ssid, s_active_password, qr, sizeof(qr))) {
        ESP_LOGE(TAG, "Wi-Fi QR text did not fit");
        return;
    }
    int mode = atomic_load(&s_mode);
    taskENTER_CRITICAL(&s_lock);
    uint32_t idle = s_policy.idle_limit_ms / 60000;
    taskEXIT_CRITICAL(&s_lock);
    display_hotspot_screen_t screen = {
        .ssid = s_active_ssid,
        .password = s_active_password,
        .wifi_qr = qr,
        .url = READ_PICO_HOTSPOT_URL,
        .idle_minutes = idle,
        .title = mode == READ_PICO_HOTSPOT_SETUP      ? "WELCOME"
                 : mode == READ_PICO_HOTSPOT_RECOVERY ? "HOME WI-FI NOT FOUND"
                                                      : NULL,
        .note = mode == READ_PICO_HOTSPOT_SETUP
                    ? "Open the page to connect your home Wi-Fi, or upload photos right away."
                : mode == READ_PICO_HOTSPOT_RECOVERY
                    ? "Retrying it in the background. Open the page to change Wi-Fi."
                    : NULL,
    };
    esp_err_t err = display_manager_show_hotspot(&screen);
    memset(qr, 0, sizeof(qr));
    if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) {
        ESP_LOGE(TAG, "Connection screen failed: %s", esp_err_to_name(err));
        return;
    }
    s_screen_shown = true;
    s_screen_done_ms = now_ms();
    // Give the user a full interval to scan before the slideshow moves on. The
    // schedule is wall-clock (cron), so a plain reset could land seconds away.
    power_manager_skip_next_rotation();
}

// The screen shows the password in clear text and e-paper keeps it without
// power, so it never stays up after the hotspot ends.
static void restore_screen(void)
{
    if (!s_screen_shown) return;
    char current[DISPLAY_IMAGE_PATH_CAPACITY];
    bool known = false;
    if (!display_manager_copy_current_image(current, sizeof(current), &known)) {
        ESP_LOGW(TAG, "Display busy; it is already replacing the connection screen");
        s_screen_shown = false;
        return;
    }
    s_screen_shown = false;
    if (current[0]) return;  // a photo already replaced the connection screen
    esp_err_t err = s_restore_path[0] ? display_manager_show_image(s_restore_path) : ESP_ERR_NOT_FOUND;
    if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED) err = display_manager_clear();
    if (err != ESP_OK && err != ESP_ERR_NOT_FINISHED)
        ESP_LOGE(TAG, "Connection screen not replaced: %s", esp_err_to_name(err));
}

static void start_radio(int64_t now)
{
    taskENTER_CRITICAL(&s_lock);
    copy_effective_ssid_locked(s_active_ssid);
    memcpy(s_active_password, s_password, sizeof(s_active_password));
    taskEXIT_CRITICAL(&s_lock);
    s_reconnect_pending = false;
    esp_err_t err = wifi_provisioning_ap_start_hotspot(s_active_ssid, s_active_password);
    taskENTER_CRITICAL(&s_lock);
    hotspot_policy_started(&s_policy, err == ESP_OK, now);
    s_last_error = err;
    taskEXIT_CRITICAL(&s_lock);
    if (err != ESP_OK) {
        // The AP lease already cancelled the station: bring it back.
        ESP_LOGE(TAG, "Start failed (%s); reconnecting the station", esp_err_to_name(err));
        s_reconnect_pending = true;
        s_reconnect_at_ms = now;
        s_reconnect_backoff_ms = HOTSPOT_RECONNECT_FIRST_MS;
        return;
    }
    ESP_LOGI(TAG, "On: %s at %s", s_active_ssid, READ_PICO_HOTSPOT_URL);
    if (atomic_load(&s_mode) == READ_PICO_HOTSPOT_RECOVERY) hotspot_retry_start(&s_retry, now);
    show_screen();
}

static void stop_radio(int64_t now)
{
    esp_err_t err = wifi_provisioning_ap_stop();
    taskENTER_CRITICAL(&s_lock);
    hotspot_policy_stopped(&s_policy, err == ESP_OK);
    hotspot_reason_t reason = s_policy.last_stop_reason;
    s_last_error = err;
    memset(s_active_password, 0, sizeof(s_active_password));
    taskEXIT_CRITICAL(&s_lock);
    atomic_store(&s_clients, 0);
    if (atomic_exchange(&s_mode, READ_PICO_HOTSPOT_MANUAL) != READ_PICO_HOTSPOT_MANUAL) {
        taskENTER_CRITICAL(&s_lock);
        hotspot_policy_set_idle_minutes(&s_policy, s_idle_minutes, now);
        taskEXIT_CRITICAL(&s_lock);
    }
    hotspot_retry_stop(&s_retry);
    restore_screen();  // even when the radio exit is unconfirmed
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Stop unconfirmed (%s); station not restarted", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Off (%s); reconnecting the station", hotspot_reason_name(reason));
    s_reconnect_pending = true;
    s_reconnect_at_ms = now;
    s_reconnect_backoff_ms = HOTSPOT_RECONNECT_FIRST_MS;
}

// Bounded like boot: a timeout hands over to background reconnecting and the
// station supervisor; an outright failure retries with a capped backoff.
static void reconnect_step(int64_t now)
{
    if (!s_reconnect_pending || now < s_reconnect_at_ms) return;
    if (wifi_manager_is_connected()) {
        s_reconnect_pending = false;
        return;
    }
    char ssid[WIFI_SSID_MAX_LEN] = {0}, password[WIFI_PASS_MAX_LEN] = {0};
    esp_err_t err = wifi_manager_load_credentials(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved station (%s); staying offline", esp_err_to_name(err));
        s_reconnect_pending = false;
        return;
    }
    err = wifi_manager_connect(ssid, password);
    memset(password, 0, sizeof(password));
    if (err == ESP_OK || err == ESP_ERR_TIMEOUT) {
        if (err == ESP_ERR_TIMEOUT) wifi_manager_keep_reconnecting();
        ESP_LOGI(TAG, "Station %s", err == ESP_OK ? "reconnected" : "still connecting in background");
        s_reconnect_pending = false;
        return;
    }
    ESP_LOGW(TAG, "Station reconnect failed (%s); retry in %u s", esp_err_to_name(err),
             (unsigned) (s_reconnect_backoff_ms / 1000));
    s_reconnect_at_ms = now_ms() + s_reconnect_backoff_ms;
    uint32_t next = s_reconnect_backoff_ms * 2;
    s_reconnect_backoff_ms = next > HOTSPOT_RECONNECT_MAX_MS ? HOTSPOT_RECONNECT_MAX_MS : next;
}

// Restart only after the AP, DNS and lease exit are confirmed, as the old portal did.
static void restart_now(bool show_setup_complete)
{
    esp_err_t err = wifi_provisioning_ap_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Hotspot exit unconfirmed (%s); no automatic restart", esp_err_to_name(err));
        s_restart_at_ms = 0;
        return;
    }
    nvs_handle_t handle;
    if (show_setup_complete && nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        if (nvs_set_u8(handle, NVS_SETUP_COMPLETE_KEY, 1) == ESP_OK) nvs_commit(handle);
        nvs_close(handle);
    }
    ESP_LOGI(TAG, "Restarting to join home Wi-Fi");
    esp_restart();
}

// Test the saved network with the AP kept up (the setup candidate path); on
// success restart into it, so nothing about the station is reconfigured live.
static void retry_home(int64_t now)
{
    char ssid[WIFI_SSID_MAX_LEN] = {0}, password[WIFI_PASS_MAX_LEN] = {0};
    wifi_network_t network;
    esp_err_t err = wifi_manager_load_credentials(ssid, password);
    if (err == ESP_OK) err = config_manager_copy_network(&network);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved Wi-Fi to retry (%s)", esp_err_to_name(err));
        hotspot_retry_stop(&s_retry);
        return;
    }
    uint32_t owner = 0;
    err = wifi_manager_test_network_owned(ssid, password, &network, &owner);
    memset(password, 0, sizeof(password));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Saved Wi-Fi answers again (attempt %u)", (unsigned) s_retry.attempts);
        restart_now(false);
        if (owner) wifi_manager_cancel_candidate(owner);  // only reached when the exit failed
        return;
    }
    hotspot_retry_failed(&s_retry, now_ms());
    ESP_LOGI(TAG, "Saved Wi-Fi still unreachable (%s); next try in %u s", esp_err_to_name(err),
             (unsigned) (s_retry.backoff_ms / 1000));
}

void read_pico_hotspot_poll(int64_t now)
{
    if (!s_ready) return;
    unsigned clients = wifi_provisioning_ap_client_count();
    atomic_store(&s_clients, clients);
    bool busy = atomic_load(&s_transfers) > 0;
    taskENTER_CRITICAL(&s_lock);
    hotspot_step_t step = hotspot_policy_step(&s_policy, now, clients, busy);
    bool on = s_policy.state == HOTSPOT_ON;
    taskEXIT_CRITICAL(&s_lock);
    if (step == HOTSPOT_STEP_START) {
        start_radio(now);
    } else if (step == HOTSPOT_STEP_STOP) {
        stop_radio(now);
    } else if (on) {
        if (atomic_exchange(&s_show_requested, false) &&
            now - s_screen_done_ms >= HOTSPOT_SCREEN_DEBOUNCE_MS)
            show_screen();
        power_manager_reset_sleep_timer();  // no battery sleep while the hotspot is up
        if (provisioning_session_complete()) {
            if (!s_restart_at_ms) {
                s_restart_at_ms = now + HOTSPOT_SAVE_RESTART_MS;
                ESP_LOGI(TAG, "Home Wi-Fi saved; restarting in %d s", HOTSPOT_SAVE_RESTART_MS / 1000);
            } else if (now >= s_restart_at_ms) {
                restart_now(true);
            }
        } else if (atomic_load(&s_mode) == READ_PICO_HOTSPOT_RECOVERY &&
                   hotspot_retry_step(&s_retry, now, clients, busy)) {
            retry_home(now);
        }
    } else {
        s_show_requested = false;
        reconnect_step(now);
    }
}
