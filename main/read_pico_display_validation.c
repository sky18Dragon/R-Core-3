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
#include "freertos/FreeRTOS.h"
#include "read_pico_board.h"
#include "read_pico_epaper.h"
#include "read_pico_scan_probe.h"
#include "read_pico_validation.h"

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 1, 0) || !CONFIG_READ_PICO_DISPLAY_VALIDATION
#error "Controlled Read Pico validation requires the explicit profile and locked SDK"
#endif

#if !CONFIG_COMPILER_OPTIMIZATION_PERF || CONFIG_ESP32S3_DATA_CACHE_SIZE != 0x10000 || \
    CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE != 64
#error \
    "Display validation requires PERF/64KB/64B; migrate stale generated display config explicitly"
#endif

static const char *TAG = "read_pico_validation";

static bool partition_matches(esp_partition_type_t type, esp_partition_subtype_t subtype,
                              const char *name, uint32_t offset, uint32_t size)
{
    const esp_partition_t *part = esp_partition_find_first(type, subtype, name);
    return part != NULL && part->address == offset && part->size == size && !part->encrypted;
}

static void memory_report(const char *phase)
{
    ESP_LOGI(TAG, "%s internal_free=%u internal_largest=%u psram_free=%u", phase,
             (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned) heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

static esp_err_t observe_exit(void)
{
    // Read-only independent evidence; no analog-voltage or visual success claim.
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
    if (err == ESP_OK && ((input & 0x20) || (output & 0x1a)))
        err = ESP_ERR_INVALID_STATE;
    return err;
}

static int clear_white(void *unused)
{
    (void) unused;
    const int64_t start = esp_timer_get_time();
    memory_report("before_init");
    // This executable/profile is used only after the explicit display-test authorization.
    // No power startup occurs at boot; a single clear-white command arms this call.
    esp_err_t err = read_pico_epaper_init(true);
    ESP_LOGI(TAG, "DISPLAY_INIT result=%s elapsed_ms=%ld", esp_err_to_name(err),
             (long) ((esp_timer_get_time() - start) / 1000));
    if (err != ESP_OK)
        return err;
    memory_report("after_init");
    err = read_pico_scan_probe_begin();
    if (err != ESP_OK)
        return err;
    ESP_LOGW(TAG, "CLEAR_BEGIN: one unverified clear, no automatic retry");
    err = read_pico_epaper_clear_unverified();
    int64_t elapsed_ms = (esp_timer_get_time() - start) / 1000;
    const read_pico_scan_capture_t *scan = read_pico_scan_probe_end();
    // Read latches promptly, before printing bounded per-frame observations.
    esp_err_t observation = observe_exit();
    if (err == ESP_OK)
        err = observation;
    read_pico_scan_summary_t summary = {0};
    bool analyzed = read_pico_scan_analyze(scan, &summary);
    ESP_LOGI(
        TAG,
        "CLEAR_SCAN software_only=1 coverage_complete=%d registered=%lu done=%lu stored=%lu "
        "lines_min=%lu lines_max=%lu missing_tail=%lu missing_padding=%lu "
        "padding_nonzero=%lu unequal_edges=%lu unexpected_pulse=%lu",
        analyzed && summary.software_complete, (unsigned long) scan->registered_frames,
        (unsigned long) scan->completed_frames, (unsigned long) scan->stored_frames,
        (unsigned long) summary.min_source_calls, (unsigned long) summary.max_source_calls,
        (unsigned long) summary.missing_tail_frames, (unsigned long) summary.missing_padding_frames,
        (unsigned long) summary.nonzero_padding_frames, (unsigned long) summary.unequal_edge_frames,
        (unsigned long) summary.unexpected_pulse_frames);
    ESP_LOGI(TAG, "CLEAR_SCAN dropped=%lu orphan=%lu late=%lu unfinished=%lu wrong_core=%lu",
             (unsigned long) scan->dropped_frames, (unsigned long) scan->orphan_lines,
             (unsigned long) scan->late_lines, (unsigned long) scan->unfinished_frames,
             (unsigned long) scan->wrong_core);
    for (uint32_t i = 0; i < scan->stored_frames; ++i) {
        const read_pico_scan_frame_t *f = &scan->frames[i];
        ESP_LOGI(TAG,
                 "CLEAR_FRAME index=%lu calls=%lu seen=0x%02x "
                 "tail_first=%02x,%02x,%02x,%02x tail_last=%02x,%02x,%02x,%02x "
                 "pad_first=%02x,%02x,%02x,%02x pad_last=%02x,%02x,%02x,%02x",
                 (unsigned long) i, (unsigned long) f->source_calls, f->seen_rows, f->first[0],
                 f->first[1], f->first[2], f->first[3], f->last[0], f->last[1], f->last[2],
                 f->last[3], f->first[4], f->first[5], f->first[6], f->first[7], f->last[4],
                 f->last[5], f->last[6], f->last[7]);
    }
    if (err == ESP_OK && (!analyzed || !summary.software_complete))
        err = ESP_ERR_INVALID_RESPONSE;
    ESP_LOGI(TAG, "CLEAR_RETURN result=%s elapsed_ms=%ld faulted=%d", esp_err_to_name(err),
             (long) elapsed_ms, read_pico_epaper_faulted());
    memory_report("after_clear");
    return err;
}

static int confirm_white(void *unused)
{
    (void) unused;
    return read_pico_epaper_confirm_white_reference();
}

static int draw_pattern(void *unused, read_pico_pattern_t pattern)
{
    (void) unused;
    memory_report("before_pattern");
    uint8_t *frame = heap_caps_malloc(READ_PICO_FRAME_BYTES, MALLOC_CAP_SPIRAM);
    if (frame == NULL)
        return ESP_ERR_NO_MEM;
    if (!read_pico_pattern_render(pattern, frame, READ_PICO_FRAME_BYTES)) {
        heap_caps_free(frame);
        return ESP_ERR_INVALID_ARG;
    }
    memory_report("pattern_allocated");
    ESP_LOGW(TAG, "GC16_BEGIN pattern=%s; one draw, then manual pattern-confirm/reject",
             read_pico_pattern_name(pattern));
    const int64_t start = esp_timer_get_time();
    read_pico_refresh_result_t result = {0};
    bool edge_probe = pattern == READ_PICO_PATTERN_GRAY16_PAGE;
    esp_err_t probe = edge_probe ? read_pico_gc16_probe_begin() : read_pico_phase_probe_begin();
    if (probe != ESP_OK) {
        heap_caps_free(frame);
        return probe;
    }
    result = pattern == READ_PICO_PATTERN_GRAY16_PAGE
                 ? read_pico_epaper_display_vendor_page(frame, READ_PICO_FRAME_BYTES)
             : pattern == READ_PICO_PATTERN_GRAY16_VENDOR
                 ? read_pico_epaper_display_vendor_gray16(frame, READ_PICO_FRAME_BYTES)
                 : read_pico_epaper_display(frame, READ_PICO_FRAME_BYTES);
    const read_pico_gc16_capture_t *gc16 = edge_probe ? read_pico_gc16_probe_end(36) : NULL;
    const read_pico_phase_capture_t *phases =
        edge_probe ? &gc16->phases : read_pico_phase_probe_end(36);
    bool faulted = read_pico_epaper_faulted();
    ESP_LOGI(TAG,
             "GC16_RETURN pattern=%s completed=%d stage=%d frame_error=%d operation=%d "
             "cleanup=%d elapsed_ms=%ld faulted=%d",
             read_pico_pattern_name(pattern), result.completed, result.error, result.frame_error,
             result.operation_error, result.cleanup_error,
             (long) ((esp_timer_get_time() - start) / 1000), faulted);
    heap_caps_free(frame);
    esp_err_t observation = observe_exit();
    memory_report("after_pattern");
    ESP_LOGI(TAG,
             "GC16_PHASE software_only=1 complete=%d registered=%lu started=%lu done=%lu "
             "stored=%lu errors=%lu dropped=%lu expected=36 leading_skip=0 prefill=32",
             phases->complete, (unsigned long) phases->registered, (unsigned long) phases->started,
             (unsigned long) phases->completed, (unsigned long) phases->stored,
             (unsigned long) phases->errors, (unsigned long) phases->dropped);
    for (uint32_t i = 0; i < phases->stored; ++i) {
        const read_pico_phase_t *p = &phases->phases[i];
        ESP_LOGI(TAG, "GC16_PHASE index=%lu prepare_us=%lld start_to_done_us=%lld gap_us=%lld",
                 (unsigned long) i, (long long) (p->start_us - p->registered_us),
                 (long long) (p->done_us - p->start_us),
                 (long long) (i ? p->registered_us - phases->phases[i - 1].done_us : 0));
    }
    read_pico_edge_summary_t edge = {0};
    bool edge_analyzed = !edge_probe || read_pico_edge_analyze(&gc16->lines, &edge);
    if (edge_probe) {
        ESP_LOGI(TAG,
                 "GC16_EDGE software_only=1 complete=%d registered=%lu done=%lu stored=%lu "
                 "lines_min=%lu lines_max=%lu missing_rows=%lu unequal_pairs=%lu "
                 "indistinguishable=0x%02x dropped=%lu orphan=%lu late=%lu unfinished=%lu "
                 "core1_start_prefill=%lu wrong_core=%lu",
                 edge_analyzed && edge.software_complete,
                 (unsigned long) gc16->lines.registered_frames,
                 (unsigned long) gc16->lines.completed_frames,
                 (unsigned long) gc16->lines.stored_frames,
                 (unsigned long) edge.min_source_calls, (unsigned long) edge.max_source_calls,
                 (unsigned long) edge.missing_rows_frames,
                 (unsigned long) edge.unequal_pair_frames, edge.indistinguishable_boundaries,
                 (unsigned long) gc16->lines.dropped_frames,
                 (unsigned long) gc16->lines.orphan_lines,
                 (unsigned long) gc16->lines.late_lines,
                 (unsigned long) gc16->lines.unfinished_frames,
                 (unsigned long) gc16->lines.core1_start_prefill,
                 (unsigned long) gc16->lines.wrong_core);
        for (unsigned i = 0; i < READ_PICO_EDGE_TRACE_ROWS; ++i) {
            uint16_t row = read_pico_edge_trace_row(i);
            ESP_LOGI(TAG, "GC16_EDGE_ROW native_y=%u logical_u=%u trajectory=%08lx", row,
                     683u - row, (unsigned long) edge.trajectory_hashes[i]);
        }
    }
    if (faulted || !result.completed || result.error != READ_PICO_REFRESH_OK ||
        result.operation_error || result.cleanup_error)
        return result.operation_error ? result.operation_error
               : result.cleanup_error ? result.cleanup_error
                                      : ESP_FAIL;
    return observation != ESP_OK ? observation
           : !phases->complete || !edge_analyzed || (edge_probe && !edge.software_complete)
               ? ESP_ERR_INVALID_RESPONSE
               : ESP_OK;
}

static int stop(void *unused)
{
    (void) unused;
    esp_err_t err = read_pico_epaper_deinit();
    ESP_LOGI(TAG, "DISPLAY_DEINIT result=%s", esp_err_to_name(err));
    if (err == ESP_OK) {
        err = read_pico_board_deinit();
        ESP_LOGI(TAG, "HAL_DEINIT result=%s", esp_err_to_name(err));
    }
    return err;
}

static const char *state_name(read_pico_validation_state_t state)
{
    switch (state) {
    case READ_PICO_VALIDATION_WAIT_CLEAR:
        return "WAIT_CLEAR_COMMAND";
    case READ_PICO_VALIDATION_WAIT_WHITE:
        return "WAIT_WHITE_CONFIRM";
    case READ_PICO_VALIDATION_REFERENCE_KNOWN:
        return "REFERENCE_KNOWN";
    case READ_PICO_VALIDATION_WAIT_PATTERN:
        return "WAIT_PATTERN_CONFIRM";
    case READ_PICO_VALIDATION_STOPPED:
        return "STOPPED";
    default:
        return "FAULT_LATCHED";
    }
}

void app_main(void)
{
    ESP_LOGW(TAG, "Controlled display validation: no SD/NVS/network/sleep/PMU calibration writes");
    ESP_LOGI(TAG, "Render profile: PERF dcache_bytes=%u line_bytes=%u (not 120MHz product timing)",
             (unsigned) CONFIG_ESP32S3_DATA_CACHE_SIZE,
             (unsigned) CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE);
    if (!partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "nvs", 0x9000,
                           0x5000) ||
        !partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, "phy_init",
                           0xe000, 0x1000) ||
        !partition_matches(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory",
                           0x10000, 0x400000) ||
        !partition_matches(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "storage",
                           0x410000, 0x500000) ||
        esp_psram_get_size() < 8388608) {
        ESP_LOGE(TAG, "Device/layout preflight failed; panel startup prohibited");
        return;
    }
    usb_serial_jtag_driver_config_t console = {.tx_buffer_size = 2048, .rx_buffer_size = 256};
    esp_err_t err =
        usb_serial_jtag_is_driver_installed() ? ESP_OK : usb_serial_jtag_driver_install(&console);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Console init failed: %s; no board startup", esp_err_to_name(err));
        return;
    }
    usb_serial_jtag_vfs_use_driver();
    err = board_hal_init();
    read_pico_board_snapshot_t pmu;
    if (err == ESP_OK)
        err = read_pico_board_get_snapshot(&pmu);
    if (err == ESP_OK && !pmu.vcom_valid)
        err = ESP_ERR_INVALID_STATE;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PMU preflight failed: %s; no panel startup", esp_err_to_name(err));
        (void) read_pico_board_deinit();
        return;
    }
    ESP_LOGI(TAG, "IDF=%s PMU=%u.%u.%u VCOM_valid=%d VCOM_mV=%d faults=0x%lx",
             esp_app_get_description()->idf_ver, pmu.identity.firmware[0], pmu.identity.firmware[1],
             pmu.identity.firmware[2], pmu.vcom_valid, pmu.vcom_mv,
             (unsigned long) pmu.status.fault_flags);
    read_pico_validation_ops_t ops = {.clear_white = clear_white,
                                      .confirm_white = confirm_white,
                                      .stop = stop,
                                      .draw_pattern = draw_pattern};
    read_pico_validation_session_t session;
    if (!read_pico_validation_init(&session, &ops)) {
        (void) read_pico_board_deinit();
        return;
    }
    read_pico_validation_reader_t reader = {0};
    ESP_LOGW(TAG, "WAIT_CLEAR_COMMAND: clear-white / white-confirm / stop / status");
    ESP_LOGW(TAG, "white-confirm requires visual white in THIS boot; no GC16 patterns auto-run");
    ESP_LOGW(
        TAG,
        "Explicit GC16: "
        "gc16-white/black/midgray/gray16/parity/corners/gray16-labeled/gray16-vendor/gray16-page; "
        "pattern-confirm/reject requires visual inspection; each pattern once");
    while (true) {
        uint8_t bytes[32];
        int count = usb_serial_jtag_read_bytes(bytes, sizeof(bytes), pdMS_TO_TICKS(100));
        for (int i = 0; i < count; ++i) {
            read_pico_validation_command_t command = read_pico_validation_feed(&reader, bytes[i]);
            if (command == READ_PICO_VALIDATION_NONE)
                continue;
            bool accepted = read_pico_validation_apply(&session, command);
            ESP_LOGW(TAG,
                     "COMMAND result=%s state=%s operation=%d pending=%s "
                     "drawn=0x%lx visually_confirmed=0x%lx",
                     accepted ? "accepted" : "rejected/failed", state_name(session.state),
                     session.operation_error, read_pico_pattern_name(session.pending_pattern),
                     (unsigned long) session.drawn_mask, (unsigned long) session.confirmed_mask);
        }
    }
}
