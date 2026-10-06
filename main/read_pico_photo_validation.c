#include "board_hal.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "image_processor.h"
#include "jpeg_file_probe.h"
#include "read_pico_board.h"
#include "read_pico_epaper.h"
#include "read_pico_photo_bridge.h"
#include "read_pico_photo_frame.h"
#include "read_pico_photo_session.h"
#include "read_pico_scan_probe.h"
#include "read_pico_sd.h"

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 1, 0) || !CONFIG_READ_PICO_PHOTO_VALIDATION
#error "Photo validation requires its explicit profile and locked SDK"
#endif
#if !CONFIG_COMPILER_OPTIMIZATION_PERF || CONFIG_ESP32S3_DATA_CACHE_SIZE != 0x10000 || \
    CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE != 64
#error "Photo validation requires the tested PERF/64KB/64B display transport"
#endif

static const char *TAG = "read_pico_photo";

static bool partition_matches(esp_partition_type_t type, esp_partition_subtype_t subtype,
                              const char *name, uint32_t offset, uint32_t size)
{
    const esp_partition_t *part = esp_partition_find_first(type, subtype, name);
    return part && part->address == offset && part->size == size && !part->encrypted;
}
static bool media_ready(void)
{
    read_pico_sd_status_t s;
    return read_pico_sd_get_status(&s) == ESP_OK && s.present && s.mounted && s.media_valid &&
           !s.exit_failed;
}
static void status(void)
{
    read_pico_sd_status_t s = {0};
    esp_err_t err = read_pico_sd_get_status(&s);
    ESP_LOGI(TAG, "SD observation=%s present=%d mounted=%d media_valid=%d exit_failed=%d",
             esp_err_to_name(err), s.present, s.mounted, s.media_valid, s.exit_failed);
}
static void memory(const char *phase)
{
    ESP_LOGI(TAG,
             "MEMORY phase=%s internal_free=%u internal_largest=%u psram_free=%u psram_largest=%u",
             phase, (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned) heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}
static esp_err_t observe_exit(void)
{
    esp_err_t err = read_pico_board_lock(100);
    uint8_t input = 0, output = 0;
    if (err == ESP_OK) {
        err = read_pico_board_ioe_read(0, &input);
        if (err == ESP_OK)
            err = read_pico_board_ioe_read(2, &output);
        read_pico_board_unlock();
    }
    ESP_LOGI(TAG, "EXIT_OBSERVATION result=%s input0=0x%02x out0=0x%02x", esp_err_to_name(err),
             input, output);
    return err != ESP_OK                         ? err
           : ((input & 0x20) || (output & 0x1a)) ? ESP_ERR_INVALID_STATE
                                                 : ESP_OK;
}
static int mount_once(void *unused)
{
    (void) unused;
    esp_err_t err = read_pico_sd_mount();
    ESP_LOGI(TAG, "Mount=%s; one attempt, no format", esp_err_to_name(err));
    status();
    return err;
}
static int clear_once(void *unused)
{
    (void) unused;
    if (!media_ready())
        return ESP_ERR_INVALID_STATE;
    memory("before_display_init");
    esp_err_t err = read_pico_epaper_init(true);
    if (err != ESP_OK)
        return err;
    err = read_pico_scan_probe_begin();
    if (err != ESP_OK)
        return err;
    err = read_pico_epaper_clear_unverified();
    const read_pico_scan_capture_t *scan = read_pico_scan_probe_end();
    read_pico_scan_summary_t summary = {0};
    bool complete = read_pico_scan_analyze(scan, &summary) && summary.software_complete;
    esp_err_t exit = observe_exit();
    ESP_LOGI(TAG,
             "CLEAR_RETURN result=%s complete=%d registered=%lu done=%lu; visual white required",
             esp_err_to_name(err), complete, (unsigned long) scan->registered_frames,
             (unsigned long) scan->completed_frames);
    memory("after_clear");
    return err != ESP_OK    ? err
           : exit != ESP_OK ? exit
           : !complete      ? ESP_ERR_INVALID_RESPONSE
                            : ESP_OK;
}
static int confirm_white(void *unused)
{
    (void) unused;
    if (!media_ready())
        return ESP_ERR_INVALID_STATE;
    return read_pico_epaper_confirm_white_reference();
}
static int show_once(void *unused)
{
    (void) unused;
    if (!media_ready())
        return ESP_ERR_INVALID_STATE;
    // SDK local low watermarks include concurrent system allocations, not just the decoder.
    esp_err_t err = heap_caps_monitor_local_minimum_free_size_start();
    if (err != ESP_OK)
        return err;
    memory("before_photo");
    size_t start_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t start_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    int64_t started = esp_timer_get_time();
    err = image_processor_process_jpeg_file_to_display(JPEG_FILE_PROBE_PATH, DITHER_FLOYD_STEINBERG,
                                                       NULL);
    ESP_LOGI(TAG, "PHOTO_PROCESS result=%s elapsed_ms=%ld reason=%s", esp_err_to_name(err),
             (long) ((esp_timer_get_time() - started) / 1000), image_processor_get_last_error());
    const uint8_t *pixels = read_pico_photo_bridge_frame();
    if (err == ESP_OK && (!pixels || !media_ready()))
        err = ESP_ERR_INVALID_STATE;
    if (err == ESP_OK)
        err = read_pico_phase_probe_begin();
    if (err == ESP_OK) {
        started = esp_timer_get_time();
        read_pico_refresh_result_t r = read_pico_epaper_display(pixels, READ_PICO_FRAME_BYTES);
        const read_pico_phase_capture_t *phases = read_pico_phase_probe_end(36);
        if (phases->complete && phases->stored == 36) {
            int64_t minimum = INT64_MAX, maximum = 0, total = 0;
            for (unsigned i = 0; i < phases->stored; ++i) {
                int64_t duration = phases->phases[i].done_us - phases->phases[i].start_us;
                if (duration < minimum)
                    minimum = duration;
                if (duration > maximum)
                    maximum = duration;
                total += duration;
            }
            ESP_LOGI(TAG,
                     "PHOTO_PHASE software_only=1 count=36 min_us=%lld max_us=%lld total_us=%lld",
                     (long long) minimum, (long long) maximum, (long long) total);
        }
        ESP_LOGI(TAG,
                 "PHOTO_GC16 completed=%d stage=%d frame_error=%d operation=%d cleanup=%d "
                 "elapsed_ms=%ld phases_complete=%d started=%lu done=%lu",
                 r.completed, r.error, r.frame_error, r.operation_error, r.cleanup_error,
                 (long) ((esp_timer_get_time() - started) / 1000), phases->complete,
                 (unsigned long) phases->started, (unsigned long) phases->completed);
        err = r.operation_error ? r.operation_error
              : r.cleanup_error ? r.cleanup_error
              : !r.completed || r.error != READ_PICO_REFRESH_OK || !phases->complete ||
                      read_pico_epaper_faulted()
                  ? ESP_FAIL
                  : ESP_OK;
        esp_err_t exit = observe_exit();
        if (err == ESP_OK)
            err = exit;
    }
    read_pico_photo_bridge_release();
    size_t low_internal = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t low_psram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG,
             "PHOTO_MEMORY local_internal_low=%u local_psram_low=%u internal_delta=%u "
             "psram_delta=%u system_inclusive=1",
             (unsigned) low_internal, (unsigned) low_psram,
             (unsigned) (start_internal > low_internal ? start_internal - low_internal : 0),
             (unsigned) (start_psram > low_psram ? start_psram - low_psram : 0));
    esp_err_t monitor = heap_caps_monitor_local_minimum_free_size_stop();
    memory("after_photo_release");
    return err != ESP_OK ? err : monitor;
}
static int stop_once(void *unused)
{
    (void) unused;
    read_pico_photo_bridge_release();
    esp_err_t display = read_pico_epaper_deinit();
    ESP_LOGI(TAG, "Display cleanup=%s", esp_err_to_name(display));
    esp_err_t sd = read_pico_sd_unmount();
    ESP_LOGI(TAG, "Unmount=%s", esp_err_to_name(sd));
    if (display != ESP_OK || sd != ESP_OK)
        return display != ESP_OK ? display : sd;
    esp_err_t hal = read_pico_board_deinit();
    ESP_LOGI(TAG, "HAL cleanup=%s", esp_err_to_name(hal));
    return hal;
}

void app_main(void)
{
    ESP_LOGW(TAG,
             "Photo diagnostic only: fixed RC3P0001.JPG, portrait/Cover/Floyd-Steinberg; no boot "
             "mount/display/NVS/network/file writes");
    if (!partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs", 0x9000,
                           0x5000) ||
        !partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, "phy_init",
                           0xe000, 0x1000) ||
        !partition_matches(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory",
                           0x10000, 0x400000) ||
        !partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "storage",
                           0x410000, 0x500000) ||
        esp_psram_get_size() < 8388608 || usb_serial_jtag_is_driver_installed()) {
        ESP_LOGE(TAG, "Layout/memory/console ownership preflight failed; no board startup");
        return;
    }
    esp_err_t err = board_hal_init();
    read_pico_board_snapshot_t pmu;
    if (err == ESP_OK)
        err = read_pico_board_get_snapshot(&pmu);
    if (err == ESP_OK && !pmu.vcom_valid)
        err = ESP_ERR_INVALID_STATE;
    if (err == ESP_OK)
        err = image_processor_init_defaults();
    usb_serial_jtag_driver_config_t console = {.tx_buffer_size = 2048, .rx_buffer_size = 256};
    if (err == ESP_OK)
        err = usb_serial_jtag_driver_install(&console);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Startup failed=%s; HAL cleanup=%s", esp_err_to_name(err),
                 esp_err_to_name(read_pico_board_deinit()));
        return;
    }
    usb_serial_jtag_vfs_use_driver();
    ESP_LOGI(TAG, "IDF=%s PMU=%u.%u.%u VCOM_mV=%d faults=0x%lx", esp_app_get_description()->idf_ver,
             pmu.identity.firmware[0], pmu.identity.firmware[1], pmu.identity.firmware[2],
             pmu.vcom_mv, (unsigned long) pmu.status.fault_flags);
    photo_ops_t ops = {.mount = mount_once,
                       .clear = clear_once,
                       .white_confirm = confirm_white,
                       .show = show_once,
                       .stop = stop_once};
    photo_session_t session;
    photo_reader_t reader = {0};
    if (!photo_session_init(&session, &ops))
        return;
    status();
    ESP_LOGW(TAG,
             "WAIT_MOUNT: status / mount / clear-white / white-confirm / photo-show / "
             "photo-confirm / photo-reject / stop; no auto commands");
    while (!session.stop_attempted) {
        uint8_t byte;
        if (usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100)) != 1)
            continue;
        photo_command_t command = photo_session_feed(&reader, byte);
        if (command == PHOTO_NONE)
            continue;
        bool accepted = photo_session_apply(&session, command);
        if (command == PHOTO_STATUS)
            status();
        ESP_LOGW(TAG, "COMMAND result=%s state=%s operation=%d cleanup=%d",
                 accepted ? "accepted" : "rejected/failed", photo_session_state_name(session.state),
                 session.operation_error, session.cleanup_error);
    }
    ESP_LOGW(TAG, "%s; command processing ended",
             session.state == PHOTO_STOPPED ? "STOPPED" : "FAILED");
    (void) usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(200));
    usb_serial_jtag_vfs_use_nonblocking();
    err = usb_serial_jtag_driver_uninstall();
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Console cleanup=%s", esp_err_to_name(err));
}
