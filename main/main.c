#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "album_manager.h"
#include "board_hal.h"
#include "color_palette.h"
#include "config.h"
#include "config_manager.h"
#include "crash_log.h"
#include "debug_log.h"
#include "display_manager.h"
#include "driver/gpio.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_dev.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// External RTC support
#ifdef CONFIG_EXT_RTC_ENABLED
#include "ext_rtc.h"
#endif

#include "ha_integration.h"
#include "http_server.h"
#include "image_processor.h"
#include "mdns_service.h"
#include "memfs.h"
#include "nvs_bootstrap.h"
#include "nvs_flash.h"
#include "ota_manager.h"
#include "periodic_tasks.h"
#include "power_manager.h"
#include "processing_settings.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include "read_pico_board.h"
#include "read_pico_background.h"
#include "read_pico_sntp.h"
#if CONFIG_READ_PICO_LOCAL_FRAME
#include "read_pico_epaper.h"
#if CONFIG_READ_PICO_LOCAL_FRAME
#include "upload_sweep.h"
#include "read_pico_ota.h"
#define UPLOAD_SWEEP_MAX_PER_BOOT 64
#endif
#if CONFIG_READ_PICO_TOUCH
#include "read_pico_touch_nav.h"
#endif
#endif
#endif
#include "splash_screen.h"
#include "storage.h"
#include "utils.h"
#include "wifi_manager.h"
#include "wifi_provisioning.h"

static const char *TAG = "main";

// Log the current wall-clock (defined below; forward-declared so the SNTP
// callback above the definition can use it too).
static void log_wall_clock(const char *label);

// Set (from the lwIP task) when SNTP actually applies a fresh time. The
// periodic sync below waits on this rather than on "is the clock already set",
// because on a timer wake the retained RTC clock is already a valid (but
// drifted) time — so it would otherwise return before the correction lands.
#if !CONFIG_BOARD_DRIVER_READ_PICO
static volatile bool s_sntp_time_applied = false;

static void sntp_time_sync_notification(struct timeval *tv)
{
    s_sntp_time_applied = true;
}
#else
static bool sntp_allowed(void)
{
    return periodic_tasks_is_active() && wifi_manager_is_connected();
}
#endif

// Periodic callback for SNTP sync
static esp_err_t sntp_sync_periodic_callback(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!sntp_allowed()) return ESP_ERR_INVALID_STATE;
    char server[NTP_SERVER_MAX_LEN];
    esp_err_t copied = config_manager_copy_ntp_server(server, sizeof(server));
    if (copied != ESP_OK) return copied;
    time_t now;
    esp_err_t synced = read_pico_sntp_sync(server, sntp_allowed, periodic_tasks_is_active, &now);
    if (synced != ESP_OK) return synced;
#else
    ESP_LOGI(TAG, "Periodic SNTP sync triggered");

    s_sntp_time_applied = false;

    // Force SNTP to sync again (timezone is already set by config_manager)
    esp_sntp_stop();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, config_manager_get_ntp_server());
    esp_sntp_setservername(1, "time.google.com");
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_time_sync_notification_cb(sntp_time_sync_notification);
    esp_sntp_init();

    // Wait for the server response to actually be APPLIED (settimeofday from the
    // lwIP task), not merely for the clock to read a valid year. Downstream, the
    // early-wake re-check re-reads the clock and depends on the corrected time
    // being in place before it decides whether the timer fired early.
    const int retry_count = 10;  // up to ~10 seconds
    for (int i = 0; i < retry_count && !s_sntp_time_applied; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (!s_sntp_time_applied) {
        ESP_LOGW(TAG, "SNTP sync timeout, will retry next period");
        return ESP_ERR_TIMEOUT;
    }

    time_t now = 0;
    time(&now);
#endif
    log_wall_clock("SNTP sync");

    // Update external RTC with synced time
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!sntp_allowed()) return ESP_ERR_INVALID_STATE;
#endif
    if (board_hal_rtc_is_available()) {
        esp_err_t ret = board_hal_rtc_set_time(now);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Updated external RTC with SNTP time");
        } else {
            ESP_LOGW(TAG, "Failed to update external RTC: %s", esp_err_to_name(ret));
#if CONFIG_BOARD_DRIVER_READ_PICO
            return ret;
#endif
        }
    }

    // Reset rotate timer now that the clock is corrected
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!sntp_allowed()) return ESP_ERR_INVALID_STATE;
#endif
    power_manager_reset_rotate_timer();

    return ESP_OK;
}

// Connect to the saved network. wifi_manager_connect() itself is bounded (it
// gives up after its retries or WIFI_CONNECT_TIMEOUT_MS), so there is nothing
// left to wait for once it returns. On ESP_ERR_TIMEOUT the link is still up and
// trying; callers that won't wait for it must call wifi_manager_stop_connecting().
static esp_err_t connect_to_wifi(bool *credentials_failed)
{
    if (credentials_failed) *credentials_failed = false;
    char wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
    char wifi_password[WIFI_PASS_MAX_LEN] = {0};

#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t loaded = wifi_manager_load_credentials(wifi_ssid, wifi_password);
    if (loaded != ESP_OK) {
        if (credentials_failed) *credentials_failed = true;
        ESP_LOGE(TAG, "Credential load failed (%s); no connect/erase/restart", esp_err_to_name(loaded));
        return loaded;
    }
#else
    ESP_ERROR_CHECK(wifi_manager_load_credentials(wifi_ssid, wifi_password));
#endif
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", wifi_ssid);
    int64_t start_us = esp_timer_get_time();
    esp_err_t err = wifi_manager_connect(wifi_ssid, wifi_password);
    int elapsed_ms = (int) ((esp_timer_get_time() - start_us) / 1000);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WiFi connected after %d ms", elapsed_ms);
    } else {
        ESP_LOGE(TAG, "WiFi connection failed after %d ms (%s)", elapsed_ms, esp_err_to_name(err));
    }
    return err;
}

// Legacy boards erase/restart; Read Pico retains credentials and uses a new
// AP save session. Failed startup/exit or cancellation returns on the new board.
static void recover_wifi_connection(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    read_pico_sntp_request_stop();
    periodic_tasks_request_stop();
    // Do not change radio/HTTP while another owned callback is still using it.
    esp_err_t background = read_pico_background_cancel_and_wait(pdMS_TO_TICKS(2000));
    if (background != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery deferred: background exit unconfirmed (%s)",
                 esp_err_to_name(background));
        return;
    }
    esp_err_t periodic_exit = periodic_tasks_stop();
    if (periodic_exit != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery deferred: periodic exit unconfirmed (%s)",
                 esp_err_to_name(periodic_exit));
        return;
    }
    esp_err_t sntp_exit = read_pico_sntp_stop();
    if (sntp_exit != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery deferred: SNTP exit unconfirmed (%s)", esp_err_to_name(sntp_exit));
        return;
    }
    // Preserve the saved pair. Re-enter the upstream captive portal in this
    // boot, waiting for a NEW completed save rather than the old NVS presence.
    esp_err_t radio_stopped = wifi_manager_stop_connecting_checked();
    if (radio_stopped != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery radio exit unconfirmed (%s); no provisioning/restart",
                 esp_err_to_name(radio_stopped));
        return;
    }
    esp_err_t stopped = http_server_stop();
    if (stopped != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery refused: HTTP stop failed (%s); credentials retained",
                 esp_err_to_name(stopped));
        return;
    }
    power_manager_set_auto_sleep_timeout(OOBE_AUTO_SLEEP_TIMEOUT_SEC);
    esp_err_t recovery = wifi_provisioning_start_ap();
    if (recovery != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery AP failed (%s); credentials retained, no automatic erase/restart",
                 esp_err_to_name(recovery));
        return;
    }
    ESP_LOGW(TAG, "WiFi failed; saved credentials retained. Waiting for a new AP configuration save");
    while (!wifi_provisioning_session_complete()) {
        if (!wifi_provisioning_is_active()) {
            ESP_LOGE(TAG, "WiFi recovery AP cancelled; no automatic restart");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_err_t ap_stopped = wifi_provisioning_stop_ap();
    if (ap_stopped != ESP_OK) {
        ESP_LOGE(TAG, "WiFi recovery AP exit unconfirmed (%s); no automatic restart",
                 esp_err_to_name(ap_stopped));
        return;
    }
    esp_restart();
#else
    ESP_LOGW(TAG, "Failed to connect to WiFi - clearing credentials");
    nvs_handle_t nvs_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        nvs_erase_key(nvs_handle, NVS_WIFI_SSID_KEY);
        nvs_erase_key(nvs_handle, NVS_WIFI_PASS_KEY);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    ESP_LOGI(TAG, "Restarting to enter provisioning mode...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
#endif
}

// Network-dependent part of an interactive boot, run once WiFi has an IP:
// inline from app_main when the connect succeeded on time, or from
// late_wifi_task when it only came up after the connect timed out. Never runs
// while offline, so it neither spends the radio on requests that cannot
// succeed nor sends the HA online notification twice.
static void startup_online_work(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!read_pico_background_running() || !http_server_is_ready()) {
        ESP_LOGE(TAG, "Online startup skipped: HTTP not ready");
        return;
    }
    char ip_str[16] = {0};
    esp_err_t address = wifi_manager_get_ip(ip_str, sizeof(ip_str));
    if (address != ESP_OK) {
        ESP_LOGE(TAG, "Online startup skipped: current address not verified (%s)", esp_err_to_name(address));
        return; // No uninitialized address log, setup NVS change or online notification.
    }
#else
    char ip_str[16];
    wifi_manager_get_ip(ip_str, sizeof(ip_str));
#endif

    // Get sanitized hostname for mDNS
#if CONFIG_BOARD_DRIVER_READ_PICO
    char device_name[DEVICE_NAME_MAX_LEN];
    if (config_manager_copy_device_name(device_name, sizeof(device_name)) != ESP_OK) {
        ESP_LOGE(TAG, "Online startup skipped: device name snapshot unavailable");
        return;
    }
#else
    const char *device_name = config_manager_get_device_name();
#endif
    char hostname[64];
    sanitize_hostname(device_name, hostname, sizeof(hostname));

    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "Web interface available at: http://%s", ip_str);
    ESP_LOGI(TAG, "Or use: http://%s.local", hostname);
    ESP_LOGI(TAG, "===========================================");

    // Show setup-complete screen if just provisioned
    nvs_handle_t nvs_handle;
    uint8_t setup_complete = 0;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
        if (nvs_get_u8(nvs_handle, NVS_SETUP_COMPLETE_KEY, &setup_complete) == ESP_OK &&
            setup_complete == 1) {
            nvs_erase_key(nvs_handle, NVS_SETUP_COMPLETE_KEY);
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "First boot after provisioning — showing setup complete screen");
            splash_screen_display_setup_complete(hostname);
        }
        nvs_close(nvs_handle);
    }

    // Notify HA that device is online (HA will poll for all data via REST API).
    // This is the always-on / cold-boot path, so the rotation-gate response
    // isn't used here.
    ESP_LOGI(TAG, "Sending online notification to Home Assistant");
    ha_notify_online(NULL);

#if CONFIG_READ_PICO_LOCAL_FRAME
    // OTA is unsupported on this board and outside R1: no delayed check.
    return;
#endif
    // Delay OTA check to avoid competing with boot-time network activity
    vTaskDelay(pdMS_TO_TICKS(10000));
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!read_pico_background_running() || !http_server_is_ready()) return;
#endif
    ota_check_for_update(NULL, 0);
}

// Interactive boot whose connect timed out (see app_main): wait for the
// network, then run the startup work the on-time path runs inline.
static void late_wifi_task(void *arg)
{
    // The disconnect handler clears WIFI_CONNECTED_BIT, but a link that
    // dropped again before this task got to run can still have woken it, so
    // confirm the link is up and otherwise wait for the next IP.
    do {
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (!read_pico_background_running() || !http_server_is_ready()) return;
        EventGroupHandle_t events = wifi_manager_get_event_group();
        if (!events) return;
        EventBits_t bits = xEventGroupWaitBits(events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                             pdFALSE, pdFALSE, pdMS_TO_TICKS(100));
#else
        EventBits_t bits =
            xEventGroupWaitBits(wifi_manager_get_event_group(), WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                pdFALSE, pdFALSE, portMAX_DELAY);
#endif
        if ((bits & WIFI_FAIL_BIT) && !wifi_manager_is_connected()) {
            // The background retries only give up when the AP keeps refusing
            // the password -- the same verdict app_main acts on at boot.
            recover_wifi_connection();
#if CONFIG_BOARD_DRIVER_READ_PICO
            return; // Failed recovery does not repeatedly start servers or erase settings.
#endif
        }
        if (!wifi_manager_is_connected()) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    } while (!wifi_manager_is_connected());
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!read_pico_background_running() || !http_server_is_ready()) return;
#endif
    ESP_LOGI(TAG, "WiFi came up late - running startup network tasks");
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t periodic = periodic_tasks_check_and_run();
#if CONFIG_READ_PICO_LOCAL_FRAME
    if (periodic != ESP_OK)
        ESP_LOGW(TAG, "R1: late time sync unavailable (%s); continuing", esp_err_to_name(periodic));
#else
    if (periodic != ESP_OK) {
        ESP_LOGE(TAG, "Late online startup skipped: periodic batch failed (%s)", esp_err_to_name(periodic));
        return;
    }
#endif
#else
    periodic_tasks_check_and_run();
#endif
    startup_online_work();
#if !CONFIG_BOARD_DRIVER_READ_PICO
    vTaskDelete(NULL);
#endif
}

static void button_task(void *arg)
{
    bool last_boot_state = 1;  // Default distinct from current to avoid triggers if NC
    if (BOARD_HAL_WAKEUP_KEY != GPIO_NUM_NC) {
        last_boot_state = gpio_get_level(BOARD_HAL_WAKEUP_KEY);
    }

    bool last_key_state = 1;
    if (BOARD_HAL_ROTATE_KEY != GPIO_NUM_NC) {
        last_key_state = gpio_get_level(BOARD_HAL_ROTATE_KEY);
    }

    bool current_boot_state, current_key_state;
    uint32_t boot_press_time = 0;
    uint32_t key_press_time = 0;

    while (1) {
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (!read_pico_background_running() || !http_server_is_ready()) return;
#endif
        if (BOARD_HAL_WAKEUP_KEY != GPIO_NUM_NC) {
            current_boot_state = gpio_get_level(BOARD_HAL_WAKEUP_KEY);

            // Handle BOOT button
            if (current_boot_state == 0 && last_boot_state == 1) {
                boot_press_time = xTaskGetTickCount();
            } else if (current_boot_state == 1 && last_boot_state == 0) {
                uint32_t duration = (xTaskGetTickCount() - boot_press_time) * portTICK_PERIOD_MS;

                if (duration > 50 && duration < 3000) {
                    ESP_LOGI(TAG, "Boot button pressed, resetting sleep timer");
                    power_manager_reset_sleep_timer();
                }
            }
            last_boot_state = current_boot_state;
        }

        if (BOARD_HAL_ROTATE_KEY != GPIO_NUM_NC) {
            current_key_state = gpio_get_level(BOARD_HAL_ROTATE_KEY);

            // Handle KEY button - trigger rotation
            if (current_key_state == 0 && last_key_state == 1) {
                key_press_time = xTaskGetTickCount();
            } else if (current_key_state == 1 && last_key_state == 0) {
                uint32_t duration = (xTaskGetTickCount() - key_press_time) * portTICK_PERIOD_MS;

                if (duration > 50 && duration < 3000) {
                    ESP_LOGI(TAG, "Key button pressed, triggering rotation");
                    power_manager_reset_sleep_timer();
                    trigger_image_rotation();
                    ha_notify_update();
                }
            }
            last_key_state = current_key_state;
        }

        if (BOARD_HAL_CLEAR_KEY != GPIO_NUM_NC) {
            bool current_clear_state = gpio_get_level(BOARD_HAL_CLEAR_KEY);
            // Handle CLEAR button (active low assumed, similar to others?)
            // Assuming standard button behavior: press = 0, release = 1
            // But verify if it's the same. Usually buttons are pulled up.

            // Static state for clear button
            static bool last_clear_state = 1;
            static uint32_t clear_press_time = 0;

            if (current_clear_state == 0 && last_clear_state == 1) {
                clear_press_time = xTaskGetTickCount();
            } else if (current_clear_state == 1 && last_clear_state == 0) {
                uint32_t duration = (xTaskGetTickCount() - clear_press_time) * portTICK_PERIOD_MS;

                if (duration > 50 && duration < 3000) {
                    ESP_LOGI(TAG, "Clear button pressed, clearing display");
                    power_manager_reset_sleep_timer();
                    display_manager_clear();
                    ha_notify_update();
                }
            }
            last_clear_state = current_clear_state;
        }

#if CONFIG_READ_PICO_TOUCH
        read_pico_touch_nav_poll(esp_timer_get_time() / 1000);
#endif
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Bring up mDNS + the HTTP config server exactly once per wake. Idempotent so
// both the pre-rotation HA path and the post-rotation config-sync window can
// call it freely without tracking who started it. (Deep sleep reboots the app,
// so the guard resets each wake.)
static esp_err_t ensure_http_server_running(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    static bool mdns_started = false;
    esp_err_t error = ESP_OK;
    if (!mdns_started) {
        error = mdns_service_init();
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "mDNS startup failed: %s; no HTTP success", esp_err_to_name(error));
            return error;
        }
        mdns_started = true;
    }
    error = http_server_init();
    if (error == ESP_OK) error = http_server_set_ready_checked();
    if (error != ESP_OK) ESP_LOGE(TAG, "HTTP startup/readiness failed: %s", esp_err_to_name(error));
    return error;
#else
    static bool started = false;
    if (started) {
        return ESP_OK;
    }
    // Start mDNS so HA / the LAN can resolve photoframe.local.
    ESP_ERROR_CHECK(mdns_service_init());
    ESP_ERROR_CHECK(http_server_init());
    http_server_set_ready();
    started = true;
    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
#endif
}

// Log the current wall-clock in a uniform, greppable format:
//   Wall-clock (<label>): <YYYY-MM-DD HH:MM:SS> (epoch <n>)
// The ESP_LOG "I (ms)" prefixes reset every boot, so each wake needs a real-time
// anchor to correlate against the server / HA logs. Logged at boot (from the
// external RTC, or the internal clock when there's no RTC) and again from the
// SNTP callback — the "SNTP sync" line is the trustworthy time on RTC-less boards.
static void log_wall_clock(const char *label)
{
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "Wall-clock (%s): %s (epoch %ld)", label, buf, (long) now);
}

void deep_sleep_wake_main(wakeup_source_t wakeup_src)
{
    bool is_button_wake = (wakeup_src == WAKEUP_SOURCE_ROTATE_BUTTON);
    // Check rotation mode and HA configuration
    rotation_mode_t rotation_mode = config_manager_get_rotation_mode();
    bool ha_configured = ha_is_configured();
    bool wifi_connected = false;

    // Early-wake check before spending power on WiFi: on boards with an
    // external RTC the corrected time is already restored at this point, so
    // a wake that fired early due to RTC drift can go back to sleep for the
    // remainder without a WiFi connection. Boards without an external RTC
    // still believe they're on time here; for them the check repeats after
    // NTP sync below.
    // Exception: ROTATE button press always rotates immediately.
    int early_seconds = power_manager_get_seconds_until_wake_target();
    if (!is_button_wake && early_seconds > EARLY_WAKE_TOLERANCE_SEC) {
        ESP_LOGI(TAG, "Woke %d seconds before scheduled rotation, going back to sleep",
                 early_seconds);
        power_manager_enter_sleep();
#if CONFIG_BOARD_DRIVER_READ_PICO
        return; // PMU sleep gate returns; do not fall through into rotation.
#endif
        // Won't reach here after sleep
    }

    // Initialize WiFi if needed (URL mode always needs it, SD card mode only if HA configured)
    if (rotation_mode == ROTATION_MODE_URL || ha_configured) {
        ESP_LOGI(TAG, "Initializing WiFi for %s",
                 rotation_mode == ROTATION_MODE_URL ? "URL rotation" : "HA battery post");
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t wifi_init = wifi_manager_init();
        if (wifi_init != ESP_OK) {
            ESP_LOGE(TAG, "Wake WiFi initialization failed (%s); no network work/restart",
                     esp_err_to_name(wifi_init));
            return;
        }
#else
        ESP_ERROR_CHECK(wifi_manager_init());
#endif

        bool credentials_failed = false;
        if (connect_to_wifi(&credentials_failed) == ESP_OK) {
            wifi_connected = true;
            ESP_LOGI(TAG, "WiFi connected");
        } else {
#if CONFIG_BOARD_DRIVER_READ_PICO
            if (credentials_failed) {
                esp_err_t stopped = wifi_manager_stop_connecting_checked();
                if (stopped != ESP_OK)
                    ESP_LOGE(TAG, "Wake radio exit unconfirmed: %s", esp_err_to_name(stopped));
                ESP_LOGE(TAG, "Wake credential read failed; no network work or automatic repair");
                return;
            }
#endif
            // This wake only needs the network up front; don't leave the radio
            // retrying through the rotation and until sleep (#121).
#if CONFIG_BOARD_DRIVER_READ_PICO
            esp_err_t stopped = wifi_manager_stop_connecting_checked();
            if (stopped != ESP_OK) {
                ESP_LOGE(TAG, "Wake radio exit unconfirmed (%s); no further wake work",
                         esp_err_to_name(stopped));
                return;
            }
#else
            wifi_manager_stop_connecting();
#endif
            ESP_LOGW(TAG, "WiFi unavailable");
        }
    }

    if (wifi_connected) {
        power_manager_reset_sleep_timer();

        // Check and run periodic tasks (OTA check, SNTP sync if due). When SNTP
        // actually corrects the clock it logs a "Wall-clock (SNTP sync)" line —
        // the trustworthy anchor on RTC-less boards.
        ESP_LOGI(TAG, "Checking periodic tasks...");
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t periodic = periodic_tasks_check_and_run();
        if (periodic != ESP_OK) {
            ESP_LOGE(TAG, "Wake periodic work failed: %s", esp_err_to_name(periodic));
            return;
        }
#else
        periodic_tasks_check_and_run();
#endif
    }

    // Re-check now that the clock is as corrected as it will get (NTP sync
    // above may have pulled it backward on boards without an external RTC).
    early_seconds = power_manager_get_seconds_until_wake_target();
    if (!is_button_wake && early_seconds > EARLY_WAKE_TOLERANCE_SEC) {
        ESP_LOGI(TAG, "Woke %d seconds before scheduled rotation, going back to sleep",
                 early_seconds);
        power_manager_enter_sleep();
#if CONFIG_BOARD_DRIVER_READ_PICO
        return; // A declined PMU sleep does not authorize more wake work.
#endif
        // Won't reach here after sleep
    }

    // Whether this wake should actually rotate. Home Assistant can veto a
    // scheduled rotation (e.g. nobody home / night) via the notify response.
    bool should_rotate = true;
    bool ha_unreachable = false;

    // URL mode without a network has nothing to do: the fetch would only
    // fail three times in a row, and the panel keeps its picture either way.
    // A scheduled wake counts it against the backoff so a frame out of WiFi
    // range stops paying for a connect attempt on every slot (#121); a
    // ROTATE press just goes back to sleep, its outcome not counted.
    if (!wifi_connected && rotation_mode == ROTATION_MODE_URL) {
        ESP_LOGW(TAG, "No network for URL rotation; keeping the current picture");
        utils_set_last_fetch_error("WiFi unavailable");
        power_manager_record_network_wake(false);
        power_manager_enter_sleep();
#if CONFIG_BOARD_DRIVER_READ_PICO
        return;
#endif
        // Won't reach here after sleep
    }

    // Bring the config server up before rotating so HA can reach us and the
    // notify response can carry the rotation decision.
    if (wifi_connected && ha_configured) {
        power_manager_reset_sleep_timer();
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (ensure_http_server_running() != ESP_OK) return;
#else
        ensure_http_server_running();
#endif

        // Piggyback the rotation decision on the online notification. The gate
        // is moot for a ROTATE button press (that always rotates), so don't ask
        // for or act on the decision there. Strictly fail-closed otherwise: WiFi
        // is up but we couldn't ask HA, so don't rotate. (A total WiFi failure
        // never reaches here, so it rotates as normal.)
        esp_err_t notify_err = ha_notify_online(is_button_wake ? NULL : &should_rotate);
        if (!is_button_wake && notify_err != ESP_OK) {
            ESP_LOGW(TAG, "Could not reach Home Assistant to check rotation; skipping");
            utils_set_last_fetch_error("Could not reach Home Assistant to check rotation");
            should_rotate = false;
            ha_unreachable = true;
        }
    }

    // Honor an HA veto (timer wakes only — a ROTATE button press always rotates).
    // A veto is a valid answer from HA and clears the backoff; not reaching HA
    // at all is a network failure and arms it.
    if (!is_button_wake && !should_rotate) {
        ESP_LOGI(TAG, "Rotation skipped by Home Assistant, going back to sleep");
        power_manager_record_network_wake(!ha_unreachable);
        power_manager_enter_sleep();
#if CONFIG_BOARD_DRIVER_READ_PICO
        return;
#endif
        // Won't reach here after sleep
    }

    // Trigger rotation. In URL mode the result says whether the fetch
    // succeeded (a 304 counts). Local rotation always succeeds, and that is
    // deliberate even when this storage-mode wake needed WiFi for HA and
    // didn't get it: the backoff skips whole wakes, and skipping these would
    // skip pictures the frame can show from its own storage to save one
    // bounded connect attempt. The backoff only takes note on a timer wake,
    // never on a ROTATE button press.
    power_manager_reset_sleep_timer();
    power_manager_record_network_wake(trigger_image_rotation() == ESP_OK);

    // Notify HA that data has been updated (after both OTA check and rotation)
    if (wifi_connected && ha_configured) {
        ha_notify_update();
    }

    // Keep the HTTP server up briefly so a late config change — or a server-side
    // config pull — can reach us. HA frames always get a short window; a server
    // that asked us to wait (X-Post-Rotate-Wait-Sec, meaning it wants to pull our
    // config) extends it, and gets a window even without HA — URL-mode frames
    // don't start the server before rotating, so we start it on demand here.
    int hold_sec = (wifi_connected && ha_configured) ? HA_CONFIG_WINDOW_SEC : 0;
    int server_wait = utils_get_post_rotate_wait_sec();
    if (server_wait > hold_sec) {
        hold_sec = server_wait;
    }
    if (wifi_connected && hold_sec > 0) {
#if CONFIG_BOARD_DRIVER_READ_PICO
        if (ensure_http_server_running() != ESP_OK) return;
#else
        ensure_http_server_running();  // no-op if the HA path already started it
#endif
        ESP_LOGI(TAG, "HTTP server available for config sync (%d s)", hold_sec);
        vTaskDelay(pdMS_TO_TICKS(hold_sec * 1000));
        ESP_LOGI(TAG, "HTTP server window closed");
    }

    // Go back to sleep (offline notification sent inside power_manager_enter_sleep)
    ESP_LOGI(TAG, "Auto-rotate complete, going back to sleep");
    power_manager_enter_sleep();
    // Won't reach here after sleep
}

#if CONFIG_BOARD_DRIVER_READ_PICO
static void photoframe_boot(void)
#else
void app_main(void)
#endif
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    // Before board/storage/file startup: never auto-erase existing Demo/calibration NVS.
    esp_err_t nvs_start = photoframe_nvs_init(false);
    if (nvs_start != ESP_OK) {
        ESP_LOGE(TAG, "NVS startup refused (%s); migration required, no erase or business startup",
                 esp_err_to_name(nvs_start));
        return;
    }
#if CONFIG_READ_PICO_LOCAL_FRAME
    // First thing after NVS: count/limit trial boots of a newly uploaded image.
    read_pico_ota_boot_guard();
#endif
    esp_err_t config_start = config_manager_init();
    if (config_start != ESP_OK) {
        ESP_LOGE(TAG, "Config load refused (%s); no board/storage/display/network startup "
                     "or automatic repair", esp_err_to_name(config_start));
        return;
    }
    char checked_ssid[WIFI_SSID_MAX_LEN], checked_password[WIFI_PASS_MAX_LEN];
    esp_err_t credential_check = wifi_manager_load_credentials(checked_ssid, checked_password);
    if (credential_check != ESP_OK && credential_check != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Credential pair refused (%s); no board/storage/network startup or repair",
                 esp_err_to_name(credential_check));
        return;
    }
#endif
    // Check reset reason to detect crashes
    esp_reset_reason_t reset_reason = esp_reset_reason();
    const char *reset_reason_str;
    switch (reset_reason) {
    case ESP_RST_POWERON:
        reset_reason_str = "Power-on reset";
        break;
    case ESP_RST_SW:
        reset_reason_str = "Software reset";
        break;
    case ESP_RST_PANIC:
        reset_reason_str = "Exception/panic";
        break;
    case ESP_RST_INT_WDT:
        reset_reason_str = "Interrupt watchdog";
        break;
    case ESP_RST_TASK_WDT:
        reset_reason_str = "Task watchdog";
        break;
    case ESP_RST_WDT:
        reset_reason_str = "Other watchdog";
        break;
    case ESP_RST_DEEPSLEEP:
        reset_reason_str = "Deep sleep wake";
        break;
    case ESP_RST_BROWNOUT:
        reset_reason_str = "Brownout reset";
        break;
    default:
        reset_reason_str = "Unknown";
        break;
    }
    ESP_LOGI(TAG, "PhotoFrame starting...");

    // Log initial memory state
    ESP_LOGI(TAG, "Free heap: %lu bytes, Largest free block: %lu bytes", esp_get_free_heap_size(),
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    // Initialize Board HAL
    ESP_LOGI(TAG, "Initializing Board HAL...");
    ESP_ERROR_CHECK(board_hal_init());

    // Initialize the storage subsystem (handles SD, LittleFS, MemFS fallbacks)
    ESP_LOGI(TAG, "Initializing storage subsystem...");
#if CONFIG_BOARD_DRIVER_READ_PICO
    // Read Pico has no fallback storage. A missing/unmountable card must not abort into a
    // panic-reboot loop: stop startup once, without retry, format or fallback.
    esp_err_t storage_start = storage_init();
    if (storage_start != ESP_OK) {
        esp_err_t hal_release = read_pico_board_deinit();
        ESP_LOGE(TAG,
                 "Storage unavailable (%s); insert/check the SD card and restart. "
                 "No retry, format or fallback. HAL release: %s",
                 esp_err_to_name(storage_start), esp_err_to_name(hal_release));
        return;
    }
#else
    ESP_ERROR_CHECK(storage_init());
#endif

    // Bring up NVS, config, and debug-log capture as early as possible — before
    // the external-RTC / I2C init below — so the persistent log covers the RTC
    // path and the wake decision, where a crash-then-reset is suspected (#105).
    // None of these depend on the RTC or the AXP2101 power-rail delay.
#if !CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t ret = photoframe_nvs_init(true);  // Preserve legacy board recovery only.
    ESP_ERROR_CHECK(ret);
#endif

#if !CONFIG_BOARD_DRIVER_READ_PICO
    ESP_ERROR_CHECK(config_manager_init());
#endif

    // Start mirroring console logs to storage if debug logging is enabled.
    debug_log_init();

    // Record the running build up front so every captured boot log is
    // self-identifying (which firmware/board produced these wakes).
    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "Firmware: %s (board %s)", app_desc->version, BOARD_HAL_NAME);
    // Capture the reset reason in the persistent log: a scheduled wake that
    // comes up as anything other than a clean deep-sleep wake (brownout, panic,
    // watchdog, power-on) lands in normal-init instead of the rotation path.
    ESP_LOGI(TAG, "Reset reason: %s", reset_reason_str);

    // A core dump from the previous boot becomes the last-crash record. Done
    // before the RTC init below, so a crash there can't loop unrecorded.
    crash_log_capture();

    // Initialize external RTC (via HAL). Kept after debug_log_init so this — the
    // suspected #105 crash site — is captured in the persistent log.
    ESP_LOGI(TAG, "Initializing RTC...");
    esp_err_t rtc_ret = board_hal_rtc_init();
    if (rtc_ret == ESP_OK) {
        ESP_LOGI(TAG, "RTC initialized successfully");
    } else if (rtc_ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "External RTC not supported on this board");
    } else {
        ESP_LOGW(TAG, "RTC initialization failed: %s", esp_err_to_name(rtc_ret));
    }

    // Wait for power rails to stabilize after AXP2101 initialization
    // The AXP2101 enables DC1, ALDO3, ALDO4 at 3.3V which power the SD card (if present)
    // Increase delay to ensure power is fully stable
    ESP_LOGI(TAG, "Waiting for power rails to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(500));  // Increased from 200ms to 500ms

    // Always restore time from external RTC (internal RTC is inaccurate)
    ESP_LOGI(TAG, "Checking external RTC for time restoration...");
    bool time_restored = false;

    if (board_hal_rtc_is_available()) {
        time_t external_time;
        esp_err_t ret = board_hal_rtc_get_time(&external_time);
        if (ret == ESP_OK) {
            struct tm external_timeinfo;
            localtime_r(&external_time, &external_timeinfo);

            if (external_timeinfo.tm_year >= (2025 - 1900)) {
                // External RTC has valid time, restore it
                struct timeval tv = {.tv_sec = external_time, .tv_usec = 0};
                settimeofday(&tv, NULL);
                time_restored = true;
            } else {
                ESP_LOGW(TAG, "External RTC time invalid (year %d)",
                         external_timeinfo.tm_year + 1900);
            }
        } else {
            ESP_LOGW(TAG, "Failed to read external RTC: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "External RTC not available, skipping time restoration");
    }

    // If external RTC failed or invalid, force SNTP sync (don't trust internal RTC)
    if (!time_restored) {
        ESP_LOGW(TAG,
                 "External RTC unavailable/invalid, will force SNTP sync after WiFi connection");
        // Force SNTP sync to run on next periodic_tasks_check_and_run()
#if !CONFIG_BOARD_DRIVER_READ_PICO
        periodic_tasks_force_run(SNTP_TASK_NAME);
#endif
    }

    // One boot-time anchor for log correlation, whatever the source. On
    // external-RTC boards this is the true wake time; on RTC-less boards it's the
    // internal clock's (untrusted) estimate — corrected by the "SNTP sync" line
    // once NTP runs.
    log_wall_clock(time_restored ? "external RTC" : "internal, pre-sync");

    // Initialize periodic tasks system
    ESP_LOGI(TAG, "Initializing periodic tasks...");
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t periodic_start = periodic_tasks_init();
    if (periodic_start == ESP_OK)
        periodic_start = periodic_tasks_register(SNTP_TASK_NAME, sntp_sync_periodic_callback, 24 * 60 * 60);
    if (periodic_start == ESP_OK && !time_restored)
        periodic_start = periodic_tasks_force_run(SNTP_TASK_NAME);
    if (periodic_start != ESP_OK) {
        ESP_LOGE(TAG, "Periodic startup refused: %s", esp_err_to_name(periodic_start));
        return;
    }
#else
    ESP_ERROR_CHECK(periodic_tasks_init());
    // Register SNTP sync as a daily task
    ESP_ERROR_CHECK(
        periodic_tasks_register(SNTP_TASK_NAME, sntp_sync_periodic_callback, 24 * 60 * 60));
#endif
    ESP_LOGI(TAG, "Registered SNTP sync as daily task");

    ESP_ERROR_CHECK(image_processor_init());

#if CONFIG_READ_PICO_LOCAL_FRAME
    esp_err_t display_start = display_manager_init();
    if (display_start == ESP_OK) display_start = read_pico_epaper_prepare_local_frame();
    if (display_start != ESP_OK) {
        ESP_LOGE(TAG, "R1 display startup refused (%s); no Web or rotation startup",
                 esp_err_to_name(display_start));
        return;
    }
#else
    ESP_ERROR_CHECK(display_manager_init());
#endif

    ESP_ERROR_CHECK(processing_settings_init());

    ESP_ERROR_CHECK(color_palette_init());

#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t power_start = power_manager_init();
    if (power_start != ESP_OK) {
        ESP_LOGE(TAG, "Power task startup failed (%s); no background activation",
                 esp_err_to_name(power_start));
        return;
    }
#else
    ESP_ERROR_CHECK(power_manager_init());
#endif

#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t metadata_start = ota_manager_init();
    if (metadata_start == ESP_OK) metadata_start = album_manager_init();
    if (metadata_start != ESP_OK) {
        ESP_LOGE(TAG, "Post-power initialization failed (%s); background remains gated",
                 esp_err_to_name(metadata_start));
        return;
    }
#if CONFIG_READ_PICO_LOCAL_FRAME
    // Before HTTP starts, so no upload can own a temporary yet: remove
    // unpublished upload temporaries left by an interrupted request.
    upload_sweep_result_t sweep;
    if (upload_sweep_orphans(IMAGE_DIRECTORY, UPLOAD_SWEEP_MAX_PER_BOOT, &sweep) == 0 &&
        (sweep.removed || sweep.failed)) {
        ESP_LOGI(TAG, "Removed %u orphaned upload temporaries (%llu bytes)%s; %u could not be removed",
                 sweep.removed, (unsigned long long) sweep.bytes,
                 sweep.truncated ? ", more remain" : "", sweep.failed);
    }
#endif
#else
    ESP_ERROR_CHECK(ota_manager_init());
    ESP_ERROR_CHECK(album_manager_init());
#endif

    // Check wake-up source
    wakeup_source_t wakeup_src = power_manager_get_wakeup_source();
    ESP_LOGI(TAG, "Wake-up source: %d", wakeup_src);

    switch (wakeup_src) {
    case WAKEUP_SOURCE_CLEAR_BUTTON:
        ESP_LOGI(TAG, "CLEAR button wakeup detected - clearing display and sleeping");
        display_manager_clear();      // Clear screen
        power_manager_enter_sleep();  // Go back to sleep
#if CONFIG_BOARD_DRIVER_READ_PICO
        return; // Sleep is gated; do not start the normal online boot afterward.
#endif
        // Won't reach here
        break;

    case WAKEUP_SOURCE_TIMER:
    case WAKEUP_SOURCE_ROTATE_BUTTON:
        ESP_LOGI(TAG, "Entering deep sleep wake path (timer or rotate button)");
        deep_sleep_wake_main(wakeup_src);
#if CONFIG_BOARD_DRIVER_READ_PICO
        return; // Also covers wake failure/declined sleep; no second startup.
#endif
        // Won't reach here after sleep
        break;

    case WAKEUP_SOURCE_BOOT_BUTTON:
        ESP_LOGI(TAG, "BOOT button wakeup detected - starting WiFi and HTTP server");
        // Continue with normal initialization
        break;

    default:
        // Cold boot or other wakeup - continue with normal initialization
        break;
    }

#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t wifi_init = wifi_manager_init();
    if (wifi_init != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed (%s); no provisioning/network restart",
                 esp_err_to_name(wifi_init));
        return;
    }
#else
    ESP_ERROR_CHECK(wifi_manager_init());
#endif
#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t provision_init = wifi_provisioning_init();
    if (provision_init != ESP_OK) {
        ESP_LOGE(TAG, "Provisioning initialization failed (%s); background remains gated",
                 esp_err_to_name(provision_init));
        return;
    }
#else
    ESP_ERROR_CHECK(wifi_provisioning_init());
#endif

#if CONFIG_BOARD_DRIVER_READ_PICO
    char loaded_ssid[WIFI_SSID_MAX_LEN], loaded_password[WIFI_PASS_MAX_LEN];
    esp_err_t provision_state = wifi_manager_load_credentials(loaded_ssid, loaded_password);
    if (provision_state != ESP_OK && provision_state != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Credential read failed (%s); no import/reprovisioning/erase/restart",
                 esp_err_to_name(provision_state));
        return;
    }
    bool provisioned = provision_state == ESP_OK;
#else
    bool provisioned = wifi_provisioning_is_provisioned();
#endif
    if (!provisioned) {
        bool creds_loaded = false;

        // Try to load WiFi credentials from storage
        char sd_ssid[WIFI_SSID_MAX_LEN] = {0};
        char sd_password[WIFI_PASS_MAX_LEN] = {0};

#if CONFIG_BOARD_DRIVER_READ_PICO
        wifi_text_t imported;
        esp_err_t import_result = storage_read_wifi_config(&imported);
        if (import_result != ESP_OK && import_result != ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "WiFi file import refused (%s); file retained, no save/AP fallback/restart",
                     esp_err_to_name(import_result));
            return;
        }
        if (import_result == ESP_OK) {
            memcpy(sd_ssid, imported.ssid, sizeof(sd_ssid));
            memcpy(sd_password, imported.password, sizeof(sd_password));
#else
        if (storage_read_wifi_credentials(sd_ssid, sd_password) == ESP_OK) {
#endif
            ESP_LOGI(TAG, "===========================================");
            ESP_LOGI(TAG, "WiFi credentials found on storage!");
            ESP_LOGI(TAG, "Saving to NVS and connecting...");
            ESP_LOGI(TAG, "===========================================");

            // Save credentials to NVS
            esp_err_t saved = wifi_manager_save_credentials(sd_ssid, sd_password);
#if CONFIG_BOARD_DRIVER_READ_PICO
            if (saved == ESP_OK && imported.has_device_name)
                saved = config_manager_set_device_name_checked(imported.device_name);
#endif
            if (saved == ESP_OK) {
                ESP_LOGI(TAG, "WiFi credentials saved to NVS");
                ESP_LOGI(TAG, "Restarting to connect with new credentials...");
                vTaskDelay(pdMS_TO_TICKS(2000));
                esp_restart();
                creds_loaded = true;
            } else {
#if CONFIG_BOARD_DRIVER_READ_PICO
                // Two credential keys plus optional name are not an atomic
                // transaction. Report possible partial save; never claim rollback.
                ESP_LOGE(TAG, "WiFi import save failed (%s); possible partial NVS result, "
                             "file retained, no AP fallback or restart", esp_err_to_name(saved));
                return;
#else
                ESP_LOGE(TAG, "Failed to save WiFi credentials to NVS");
#endif
            }
        }

        // No credentials found, start captive portal provisioning
        if (!creds_loaded) {
            ESP_LOGI(TAG, "===========================================");
            ESP_LOGI(TAG, "No WiFi credentials found - Starting AP mode");
            ESP_LOGI(TAG, "===========================================");

            // Out-of-box setup: give the user more time to scan the QR code
            // and provision via the app before the device auto-sleeps.
            power_manager_set_auto_sleep_timeout(OOBE_AUTO_SLEEP_TIMEOUT_SEC);

            // Show OOBE splash screen with WiFi QR code
            splash_screen_display();

            if (storage_has_persistent_storage()) {
                ESP_LOGI(TAG, "Option 1: Place wifi.txt on root of storage with:");
                ESP_LOGI(TAG, "  Line 1: WiFi SSID");
                ESP_LOGI(TAG, "  Line 2: WiFi Password");
                ESP_LOGI(TAG, "  Line 3: Device Name (optional, default: PhotoFrame)");
                ESP_LOGI(TAG, "  Then restart the device");
                ESP_LOGI(TAG, "===========================================");
            }
            ESP_LOGI(TAG, "Option 2: Use captive portal:");
            ESP_LOGI(TAG, "1. Connect to WiFi: PhotoFrame - XXXXXX");
            ESP_LOGI(TAG, "2. Open browser to: http://192.168.4.1");
            ESP_LOGI(TAG, "3. Enter your WiFi credentials");
            ESP_LOGI(TAG, "===========================================");

#if CONFIG_BOARD_DRIVER_READ_PICO
            esp_err_t ap_start = wifi_provisioning_start_ap();
            if (ap_start != ESP_OK) {
                ESP_LOGE(TAG, "Provisioning AP failed (%s); no automatic erase/restart",
                         esp_err_to_name(ap_start));
                return;
            }
#else
            ESP_ERROR_CHECK(wifi_provisioning_start_ap());
#endif

            while (!wifi_provisioning_session_complete()) {
#if CONFIG_BOARD_DRIVER_READ_PICO
                if (!wifi_provisioning_is_active()) {
                    ESP_LOGE(TAG, "Provisioning AP cancelled; no automatic restart");
                    return;
                }
#endif
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            // Set flag to show setup-complete screen after restart
            nvs_handle_t nvs_handle;
            if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
                nvs_set_u8(nvs_handle, NVS_SETUP_COMPLETE_KEY, 1);
                nvs_commit(nvs_handle);
                nvs_close(nvs_handle);
            }

            ESP_LOGI(TAG, "WiFi credentials saved! Restarting...");
            vTaskDelay(pdMS_TO_TICKS(3000));
#if CONFIG_BOARD_DRIVER_READ_PICO
            esp_err_t ap_stopped = wifi_provisioning_stop_ap();
            if (ap_stopped != ESP_OK) {
                ESP_LOGE(TAG, "Provisioning AP exit unconfirmed (%s); no automatic restart",
                         esp_err_to_name(ap_stopped));
                return;
            }
#endif
            esp_restart();
        }
    }

    bool credentials_failed = false;
    esp_err_t wifi_err = connect_to_wifi(&credentials_failed);
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (credentials_failed) return; // Read errors are not AP rejections or absent configuration.
#endif
    if (wifi_err == ESP_OK) {
        // Check and run periodic tasks (OTA check, SNTP sync if due)
        // Note: If RTC was invalid at boot, sntp_sync was already forced via
        // periodic_tasks_force_run()
        ESP_LOGI(TAG, "Checking periodic tasks...");
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t periodic = periodic_tasks_check_and_run();
#if CONFIG_READ_PICO_LOCAL_FRAME
        // No external RTC: SNTP is forced every boot and fails on a LAN without Internet.
        // Local Web/SD rotation does not need wall-clock time, so keep starting HTTP; the
        // forced request stays pending for the hourly worker. Cancellation is still caught
        // by the background/HTTP readiness checks below.
        if (periodic != ESP_OK)
            ESP_LOGW(TAG, "R1: time sync unavailable (%s); continuing with local Web/SD frame",
                     esp_err_to_name(periodic));
#else
        if (periodic != ESP_OK) {
            ESP_LOGE(TAG, "Online startup refused: periodic work failed (%s)", esp_err_to_name(periodic));
            return;
        }
#endif
#else
        periodic_tasks_check_and_run();
#endif

        // Start mDNS service
#if CONFIG_BOARD_DRIVER_READ_PICO
        esp_err_t mdns_started = mdns_service_init();
        if (mdns_started != ESP_OK) {
            ESP_LOGE(TAG, "mDNS startup failed (%s); no HTTP/startup success", esp_err_to_name(mdns_started));
            return;
        }
#else
        ESP_ERROR_CHECK(mdns_service_init());
#endif
    } else if (wifi_err == ESP_ERR_TIMEOUT) {
        // Slow rather than failed: e.g. associated but DHCP hasn't answered,
        // or the AP is still booting after a power cut. That says nothing
        // about the saved credentials being wrong, so keep them and keep
        // trying in the background: a battery frame's auto-sleep timer still
        // ends this wake, while a USB-powered or always-on frame, which never
        // auto-sleeps, comes online when the network does instead of staying
        // offline until someone power-cycles it. mDNS is started now so it
        // announces as soon as an address arrives; the rest of the online
        // work runs from late_wifi_task.
        ESP_LOGW(TAG, "WiFi connect timed out - keeping credentials, still trying");
        wifi_manager_keep_reconnecting();
        if (mdns_service_init() != ESP_OK) {
            ESP_LOGW(TAG, "mDNS not started; the frame is reachable by IP only");
        }
    } else {
        recover_wifi_connection();
#if CONFIG_BOARD_DRIVER_READ_PICO
        return; // Recovery owns its AP and restart; do not start a second HTTP server.
#endif
    }

#if CONFIG_BOARD_DRIVER_READ_PICO
    esp_err_t background_start = read_pico_background_add(READ_PICO_BACKGROUND_BUTTON,
                                                          button_task, NULL, 8192, 5);
    uint32_t required = READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_PERIODIC) |
                        READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_ROTATION) |
                        READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_BUTTON);
    if (background_start == ESP_OK && wifi_err != ESP_OK) {
        required |= READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_LATE_WIFI);
        background_start = read_pico_background_add(READ_PICO_BACKGROUND_LATE_WIFI,
                                                     late_wifi_task, NULL, 8192, 5);
    }
    if (background_start != ESP_OK) {
        ESP_LOGE(TAG, "Background creation failed (%s); no new HTTP publication",
                 esp_err_to_name(background_start));
        return;
    }
    esp_err_t http_started = http_server_init();
    if (http_started == ESP_OK)
        http_started = read_pico_background_activate(required, http_server_set_ready_checked);
    if (http_started != ESP_OK) {
        read_pico_background_cancel();
        esp_err_t http_exit = http_server_stop();
        ESP_LOGE(TAG, "HTTP startup/readiness failed (%s); no startup success or late WiFi task execution",
                 esp_err_to_name(http_started));
        if (http_exit != ESP_OK)
            ESP_LOGE(TAG, "HTTP startup cleanup unconfirmed: %s", esp_err_to_name(http_exit));
        return;
    }
#else
    xTaskCreate(button_task, "button_task", 8192, NULL, 5, NULL);
    ESP_ERROR_CHECK(http_server_init());
    http_server_set_ready();
#endif

#if CONFIG_BOARD_DRIVER_READ_PICO
    if (!read_pico_background_running() || !http_server_is_ready()) return;
#endif
    ESP_LOGI(TAG, "PhotoFrame started successfully");

    if (wifi_err == ESP_OK) {
        startup_online_work();
    } else {
#if !CONFIG_BOARD_DRIVER_READ_PICO
        // ESP_ERR_TIMEOUT: late_wifi_task takes over once the network is up.
        xTaskCreate(late_wifi_task, "late_wifi", 8192, NULL, 5, NULL);
#endif
    }
}

#if CONFIG_BOARD_DRIVER_READ_PICO
void app_main(void)
{
    photoframe_boot();
    if (!read_pico_background_running()) {
        read_pico_sntp_request_stop();
        periodic_tasks_request_stop();
        read_pico_background_cancel();
        esp_err_t exited = read_pico_background_cancel_and_wait(pdMS_TO_TICKS(2000));
        if (exited != ESP_OK) {
            ESP_LOGE(TAG, "Background exit unconfirmed (%s); no further HTTP cleanup",
                     esp_err_to_name(exited));
            return;
        }
        esp_err_t periodic_exit = periodic_tasks_stop();
        if (periodic_exit != ESP_OK) {
            ESP_LOGE(TAG, "Periodic exit unconfirmed (%s); no further HTTP cleanup",
                     esp_err_to_name(periodic_exit));
            return;
        }
        esp_err_t sntp_exit = read_pico_sntp_stop();
        if (sntp_exit != ESP_OK) {
            ESP_LOGE(TAG, "SNTP exit unconfirmed (%s); no further HTTP cleanup", esp_err_to_name(sntp_exit));
            return;
        }
        // A wake config window may have published FRAME HTTP without starting
        // normal background tasks. Close it only after owned callbacks return.
        if (http_server_is_ready()) {
            esp_err_t http_exit = http_server_stop();
            if (http_exit != ESP_OK)
                ESP_LOGE(TAG, "Boot-return HTTP exit unconfirmed: %s", esp_err_to_name(http_exit));
        }
        ESP_LOGW(TAG, "Background startup not active; cooperative cancellation requested, "
                      "not whole-device cleanup");
    }
}
#endif
