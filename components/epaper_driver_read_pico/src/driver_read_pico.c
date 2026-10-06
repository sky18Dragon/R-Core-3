#include <string.h>

#include "driver/gpio.h"
#include "e0470_epaper_waveform.h"
#include "epaper.h"
#include "epdiy.h"
#include "esp_clk_tree.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_rom_gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "read_pico_board.h"
#include "read_pico_epaper.h"
#include "read_pico_epd_timing.h"
#include "read_pico_sy_power.h"
#include "soc/gpio_sig_map.h"
#include "soc/clk_tree_defs.h"
#include "sy7636a.h"

#if CONFIG_READ_PICO_DISPLAY_VALIDATION
#include "read_pico_vendor_gray16.h"
#endif

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 1, 0)
#error "Read Pico display integration requires ESP-IDF v6.1.0"
#endif

static const char *TAG = "epaper_read_pico";
// Match the locked Read Pico operating point. The shared timing solver must
// re-derive blanking and CKV from this value; do not change only the LCD clock.
// Leading-skip remains disabled so this candidate changes one scan variable.
#define READ_PICO_DISPLAY_PCLK_MHZ 18
static StaticSemaphore_t display_mutex_storage;
static SemaphoreHandle_t display_mutex;
static read_pico_sy_handle_t sy;
static EpdiyHighlevelState hl;
static bool initialized, renderer_initialized, lease, rails, fault, reference_known, white_pending;
static bool untouched_white;
static esp_pm_lock_handle_t cpu_pm_lock, apb_pm_lock;
static bool pm_locks_held;
static esp_err_t callback_error;
static int temperature_c;

static void read_scan_clocks(uint32_t *cpu_hz, uint32_t *apb_hz)
{
    *cpu_hz = 0;
    *apb_hz = 0;
    (void) esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU,
                                        ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, cpu_hz);
    (void) esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_APB,
                                        ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, apb_hz);
}

static esp_err_t hold_scan_clocks(void)
{
    if (pm_locks_held || cpu_pm_lock == NULL || apb_pm_lock == NULL)
        return ESP_ERR_INVALID_STATE;
    uint32_t cpu_before_hz, apb_before_hz;
    read_scan_clocks(&cpu_before_hz, &apb_before_hz);
    esp_err_t err = esp_pm_lock_acquire(cpu_pm_lock);
    if (err != ESP_OK)
        return err;
    err = esp_pm_lock_acquire(apb_pm_lock);
    if (err != ESP_OK) {
        (void) esp_pm_lock_release(cpu_pm_lock);
        return err;
    }
    pm_locks_held = true;
    uint32_t cpu_after_hz, apb_after_hz;
    read_scan_clocks(&cpu_after_hz, &apb_after_hz);
    ESP_LOGI(TAG, "scan clock lock: CPU %u->%u MHz APB %u->%u MHz",
             (unsigned) (cpu_before_hz / 1000000), (unsigned) (cpu_after_hz / 1000000),
             (unsigned) (apb_before_hz / 1000000), (unsigned) (apb_after_hz / 1000000));
    return ESP_OK;
}

static esp_err_t release_scan_clocks(void)
{
    if (!pm_locks_held)
        return ESP_OK;
    esp_err_t err = esp_pm_lock_release(apb_pm_lock);
    esp_err_t cpu_err = esp_pm_lock_release(cpu_pm_lock);
    pm_locks_held = false;
    return err != ESP_OK ? err : cpu_err;
}

static esp_err_t delete_scan_locks(void)
{
    if (pm_locks_held)
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = ESP_OK;
    if (apb_pm_lock != NULL) {
        err = esp_pm_lock_delete(apb_pm_lock);
        if (err == ESP_OK)
            apb_pm_lock = NULL;
    }
    if (err == ESP_OK && cpu_pm_lock != NULL) {
        err = esp_pm_lock_delete(cpu_pm_lock);
        if (err == ESP_OK)
            cpu_pm_lock = NULL;
    }
    return err;
}

static void capture(esp_err_t err)
{
    if (callback_error == ESP_OK && err != ESP_OK)
        callback_error = err;
}

static esp_err_t set_en(bool on)
{
    esp_err_t err = read_pico_board_ioe_update(0x08, on ? 0x08 : 0);
    capture(err);
    return err;
}
static esp_err_t set_vcom_en(bool on)
{
    esp_err_t err = read_pico_board_ioe_update(0x10, on ? 0x10 : 0);
    capture(err);
    return err;
}
static int pgood(void)
{
    uint8_t input = 0;
    esp_err_t err = read_pico_board_ioe_read(0, &input);
    capture(err);
    return err == ESP_OK ? ((input & 0x20) ? 1 : 0) : -1;
}
static void set_ctrl(epd_ctrl_state_t *state, const epd_ctrl_state_t *mask)
{
    uint8_t bits = (mask->ep_mode ? 1 : 0) | (mask->ep_output_enable ? 2 : 0);
    uint8_t value = (state->ep_mode ? 1 : 0) | (state->ep_output_enable ? 2 : 0);
    if (bits)
        capture(read_pico_board_ioe_update(bits, value & bits));
}

static lcd_bus_config_t lcd_bus = {
    .clock = GPIO_NUM_21,
    .ckv = GPIO_NUM_48,
    .leh = GPIO_NUM_3,
    .start_pulse = GPIO_NUM_46,
    .stv = GPIO_NUM_47,
    .data = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 45},
};
static void board_init(uint32_t row_width)
{
    if (row_width != READ_PICO_PANEL_WIDTH) {
        capture(ESP_ERR_INVALID_SIZE);
        return;
    }
    read_pico_epd_scan_t scan;
    read_pico_epd_scan(READ_PICO_DISPLAY_PCLK_MHZ, READ_PICO_EPD_SCAN_FULL, &scan);
    read_pico_epd_init_lcd(&lcd_bus, 16, 1216, 684, &scan);
}
static void board_deinit(void)
{
    epd_lcd_deinit();
}

static esp_err_t stop_rails(void)
{
    // Outer SY adapter propagates all shutdown errors; also verify output latches and PGOOD.
    // Collect shutdown errors separately, then restore any earlier operation error so a
    // vendor poweroff callback (e.g. mid-draw) can never erase it and report success.
    const esp_err_t prior_error = callback_error;
    callback_error = ESP_OK;
    esp_err_t err = read_pico_board_ioe_update(0x03, 0x01);
    if (sy != NULL) {
        esp_err_t sy_err = read_pico_sy_power_off(sy);
        if (err == ESP_OK)
            err = sy_err;
    }
    esp_err_t force_off = read_pico_board_ioe_update(0x18, 0);
    if (err == ESP_OK)
        err = force_off;
    uint8_t output = 0, input = 0;
    esp_err_t verify = read_pico_board_ioe_read(2, &output);
    if (verify == ESP_OK)
        verify = read_pico_board_ioe_read(0, &input);
    if (verify == ESP_OK && ((output & 0x1a) || (input & 0x20)))
        verify = ESP_ERR_INVALID_STATE;
    if (err == ESP_OK)
        err = verify;
    if (err == ESP_OK)
        err = callback_error;
    callback_error = prior_error != ESP_OK ? prior_error : err;
    rails = false;
    if (err != ESP_OK)
        fault = true;
    return err;
}
static void poweron(epd_ctrl_state_t *state)
{
    if (fault || sy == NULL) {
        capture(ESP_ERR_INVALID_STATE);
        return;
    }
    esp_rom_gpio_connect_out_signal(GPIO_NUM_46, LCD_H_ENABLE_IDX, false, false);
    state->ep_mode = true;
    state->ep_output_enable = false;
    epd_ctrl_state_t mask = {.ep_mode = true, .ep_output_enable = true};
    set_ctrl(state, &mask);
    if (callback_error != ESP_OK)
        return;
    capture(read_pico_sy_power_on(sy));
    if (callback_error != ESP_OK)
        return;
    state->ep_output_enable = true;
    set_ctrl(state, &mask);
    rails = callback_error == ESP_OK;
}
static void poweroff(epd_ctrl_state_t *state)
{
    state->ep_output_enable = false;
    state->ep_mode = true;
    capture(stop_rails());
}
static float board_temperature(void)
{
    return (float) temperature_c;
}
// No set_vcom/measure_vcom callback: PMU calibration is never written.
static const EpdBoardDefinition board = {
    .init = board_init,
    .deinit = board_deinit,
    .set_ctrl = set_ctrl,
    .poweron = poweron,
    .poweroff = poweroff,
    .get_temperature = board_temperature,
};

static bool acquire(void *context)
{
    (void) context;
    if (display_mutex == NULL || xSemaphoreTake(display_mutex, 0) != pdTRUE)
        return false;
    if (read_pico_board_lock(0) != ESP_OK) {
        xSemaphoreGive(display_mutex);
        return false;
    }
    return true;
}
static void release(void *context)
{
    (void) context;
    read_pico_board_unlock();
    xSemaphoreGive(display_mutex);
}
static int prepare(void *context, int vcom_mv)
{
    (void) context;
    if (!initialized || fault || !reference_known || sy == NULL ||
        read_pico_sy_get_vcom(sy) != vcom_mv)
        return ESP_ERR_INVALID_STATE;
    esp_err_t lock_err = hold_scan_clocks();
    if (lock_err != ESP_OK)
        return lock_err;
    callback_error = ESP_OK;
    epd_poweron();
    int pg = (callback_error == ESP_OK && rails) ? pgood() : -1;
    if (callback_error != ESP_OK || !rails || pg != 1)
        return callback_error != ESP_OK ? callback_error : ESP_ERR_INVALID_STATE;
    sy7636a_status_t state;
    esp_err_t err = read_pico_sy_read(sy, &state);
    if (err != ESP_OK || !state.on || !state.pg || state.fault != SY7636A_FAULT_NONE)
        return err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
    return ESP_OK;
}
static int draw_frame(const uint8_t *frame, size_t size, int temperature, unsigned vendor_gray16)
{
    if (size != READ_PICO_FRAME_BYTES || frame != hl.front_fb)
        return ESP_ERR_INVALID_ARG;
    enum EpdDrawError draw;
#if CONFIG_READ_PICO_DISPLAY_VALIDATION
    if (vendor_gray16) {
        if (!untouched_white ||
            !(vendor_gray16 == 2
                  ? read_pico_vendor_gray16_page_render(hl.difference_fb, READ_PICO_FRAME_BYTES)
                  : read_pico_vendor_gray16_render(hl.difference_fb, READ_PICO_FRAME_BYTES)) ||
            memcmp(hl.front_fb, hl.difference_fb, READ_PICO_FRAME_BYTES) != 0)
            return ESP_ERR_INVALID_STATE;
        // difference_fb is scratch until HL builds its difference image. The target now
        // comes from real vendor epd_fill_rect, not the portable conversion output.
        memcpy(hl.front_fb, hl.difference_fb, READ_PICO_FRAME_BYTES);
        ESP_LOGI(TAG, "GC16 vendor control: i*17 epd_fill_rect native_equal=1 from_white=1");
        draw = epd_hl_update_screen_from_white(&hl, MODE_GC16, temperature);
    } else
#else
    (void) vendor_gray16;
#endif
    {
        draw = epd_hl_update_screen_full(&hl, MODE_GC16, temperature);
    }
    untouched_white = false;
    // The vendor HL API updates back_fb even on a draw error. Save only the logical reference;
    // on any error latch the panel unknown and prohibit further normal refreshes this boot.
    if (draw != EPD_DRAW_SUCCESS || callback_error != ESP_OK) {
        fault = true;
        reference_known = false;
        return callback_error != ESP_OK ? callback_error : (int) draw;
    }
    return ESP_OK;
}
static int draw_gc16(void *context, const uint8_t *frame, size_t size, int temperature)
{
    (void) context;
    return draw_frame(frame, size, temperature, false);
}
#if CONFIG_READ_PICO_DISPLAY_VALIDATION
static int draw_vendor_gc16(void *context, const uint8_t *frame, size_t size, int temperature)
{
    (void) context;
    return draw_frame(frame, size, temperature, true);
}
static int draw_vendor_page(void *context, const uint8_t *frame, size_t size, int temperature)
{
    (void) context;
    return draw_frame(frame, size, temperature, 2);
}
#endif
static int read_active_temperature(void *context, int *temperature)
{
    (void) context;
    if (temperature == NULL || !rails || callback_error != ESP_OK)
        return ESP_ERR_INVALID_STATE;
    // SY7636A Rev.0.9B: conversion runs only with ON=1. Standard power_on has
    // already waited for PGOOD and 5 ms VCOM settle; never use the standby reset value.
    sy7636a_status_t state;
    esp_err_t err = read_pico_sy_read(sy, &state);
    if (err != ESP_OK)
        return err;
    int pg = pgood();
    if (callback_error != ESP_OK)
        return callback_error;
    if (!state.on || !state.pg || state.fault != SY7636A_FAULT_NONE || pg != 1 ||
        state.temperature_c < 0 || state.temperature_c > 50)
        return ESP_ERR_INVALID_STATE;
    temperature_c = state.temperature_c;
    *temperature = temperature_c;
    ESP_LOGI(TAG, "GC16 active temperature=%d C ON=1 PGOOD=1 (not standby sample)", temperature_c);
    return ESP_OK;
}
static int stop(void *context)
{
    (void) context;
    esp_err_t err = stop_rails();
    esp_err_t unlock_err = release_scan_clocks();
    return err != ESP_OK ? err : unlock_err;
}
static const read_pico_refresh_ops_t ops = {
    .try_acquire = acquire,
    .release = release,
    .prepare = prepare,
    .draw_gc16 = draw_gc16,
    .stop = stop,
    .read_temperature = read_active_temperature,
};

static void free_buffers(void)
{
    heap_caps_free(hl.front_fb);
    heap_caps_free(hl.back_fb);
    heap_caps_free(hl.difference_fb);
    heap_caps_free(hl.dirty_lines);
    heap_caps_free(hl.dirty_columns);
    memset(&hl, 0, sizeof(hl));
}
static esp_err_t allocate_buffers(void)
{
    hl.front_fb = heap_caps_aligned_alloc(16, READ_PICO_FRAME_BYTES, MALLOC_CAP_SPIRAM);
    hl.back_fb = heap_caps_aligned_alloc(16, READ_PICO_FRAME_BYTES, MALLOC_CAP_SPIRAM);
    hl.difference_fb = heap_caps_aligned_alloc(16, READ_PICO_FRAME_BYTES * 2, MALLOC_CAP_SPIRAM);
    hl.dirty_lines = heap_caps_calloc(684, sizeof(bool), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    hl.dirty_columns = heap_caps_aligned_alloc(16, 608, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    hl.waveform = &E0470_WAVEFORM;
    if (!hl.front_fb || !hl.back_fb || !hl.difference_fb || !hl.dirty_lines || !hl.dirty_columns) {
        free_buffers();
        return ESP_ERR_NO_MEM;
    }
    memset(hl.front_fb, 0xff, READ_PICO_FRAME_BYTES);
    memset(hl.back_fb, 0xff, READ_PICO_FRAME_BYTES);
    return ESP_OK;
}

static esp_err_t release_lease(void)
{
    esp_err_t err = read_pico_board_display_release();
    if (err == ESP_OK)
        lease = false;
    else
        fault = true;
    return err;
}

esp_err_t read_pico_epaper_init(bool authorized)
{
    if (!authorized)
        return ESP_ERR_NOT_SUPPORTED;
    if (display_mutex == NULL)
        display_mutex = xSemaphoreCreateMutexStatic(&display_mutex_storage);
    if (!acquire(NULL))
        return ESP_ERR_TIMEOUT;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (initialized || fault || renderer_initialized || lease)
        goto done;
    read_pico_board_snapshot_t pmu;
    err = read_pico_board_refresh();
    if (err == ESP_OK)
        err = read_pico_board_get_snapshot(&pmu);
    if (err == ESP_OK && !pmu.vcom_valid)
        err = ESP_ERR_INVALID_STATE;
    if (err != ESP_OK)
        goto done;
    i2c_master_bus_handle_t bus;
    err = read_pico_board_display_claim(&bus);
    if (err != ESP_OK)
        goto done;
    lease = true;
    callback_error = ESP_OK;
    err = read_pico_board_ioe_configure_display();
    if (err != ESP_OK) {
        // No startup was attempted. Do not commandeer pre-existing/unknown live rails.
        (void) release_lease();
        goto done;
    }
    err = read_pico_sy_init(bus, pmu.vcom_mv, set_en, set_vcom_en, pgood, &sy);
    if (err != ESP_OK)
        goto fail;
    err = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "read_pico_epd_cpu", &cpu_pm_lock);
    if (err != ESP_OK)
        goto fail;
    err = esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "read_pico_epd_apb", &apb_pm_lock);
    if (err != ESP_OK)
        goto fail;
    err = allocate_buffers();
    if (err != ESP_OK)
        goto fail;
    // Vendor renderer/LCD init contains assert/abort allocation paths. Reserve generous
    // startup headroom, but do not advertise this preflight as an allocation-failure guarantee.
    if (heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) < 65536) {
        err = ESP_ERR_NO_MEM;
        goto fail;
    }
    e0470_waveform_init();
    epd_init(&board, &E0470_DISPLAY, EPD_LUT_1K);
    renderer_initialized = true;
    read_pico_epd_set_pclk(READ_PICO_DISPLAY_PCLK_MHZ);
    epd_set_rotation(EPD_ROT_LANDSCAPE);
    epd_set_leading_skip(false);
    if (callback_error != ESP_OK) {
        err = callback_error;
        fault = true;
        goto done;
    }
    initialized = true;
    err = ESP_OK;
    goto done;
fail:
    // Keep a failed lease/resources if cleanup cannot prove rails off; deinit may retry.
    {
        esp_err_t cleanup = stop_rails();
        if (cleanup == ESP_OK) {
            cleanup = read_pico_sy_deinit(&sy);
            if (cleanup == ESP_OK) {
                cleanup = delete_scan_locks();
                if (cleanup == ESP_OK) {
                    free_buffers();
                    (void) release_lease();
                }
            } else
                fault = true;
        } else {
            fault = true;
        }
    }
done:
    release(NULL);
    return err;
}

static bool borrow_lock(void *unused)
{
    (void) unused;
    return true;
}
static void keep_lock(void *unused)
{
    (void) unused;
}

static read_pico_refresh_result_t display_frames(const uint8_t *frame, size_t size,
                                                 const uint8_t *second_frame, size_t second_size,
                                                 unsigned vendor_gray16)
{
    read_pico_refresh_result_t failed = {.error = READ_PICO_REFRESH_GATE_CLOSED};
    if (frame == NULL || size != READ_PICO_FRAME_BYTES ||
        (second_frame != NULL && second_size != READ_PICO_FRAME_BYTES)) {
        failed.error = READ_PICO_REFRESH_BAD_FRAME;
        size_t bad_size = size != READ_PICO_FRAME_BYTES ? size : second_size;
        failed.frame_error = frame == NULL || (second_frame == NULL && second_size != 0)
                                 ? READ_PICO_FRAME_INVALID_ARGUMENT
                             : bad_size < READ_PICO_FRAME_BYTES ? READ_PICO_FRAME_BUFFER_TOO_SMALL
                                                                : READ_PICO_FRAME_INVALID_ARGUMENT;
        return failed;
    }
    if (!acquire(NULL)) {
        failed.error = READ_PICO_REFRESH_BUSY;
        return failed;
    }
    if (!initialized || fault || !reference_known || (vendor_gray16 && !untouched_white)) {
        release(NULL);
        return failed;
    }
    read_pico_board_snapshot_t pmu;
    esp_err_t err = read_pico_board_refresh();
    if (err == ESP_OK)
        err = read_pico_board_get_snapshot(&pmu);
    if (err != ESP_OK || !pmu.vcom_valid || pmu.vcom_mv != read_pico_sy_get_vcom(sy)) {
        release(NULL);
        failed.operation_error = err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
        return failed;
    }
    // Standby reads verify OFF/fault state only. EN low resets the temperature register;
    // temperature conversion needs ON=1, so sampling is deferred to the prepared rails.
    callback_error = ESP_OK;
    bool woke = false;
    sy7636a_status_t status = {0};
    err = read_pico_sy_i2c_begin(sy, &woke);
    if (err == ESP_OK)
        err = read_pico_sy_read(sy, &status);
    esp_err_t end_err = read_pico_sy_i2c_end(sy, woke);
    if (err == ESP_OK)
        err = end_err;
    if (err == ESP_OK)
        err = callback_error;
    if (err != ESP_OK || status.on || status.fault != SY7636A_FAULT_NONE) {
        if (callback_error != ESP_OK ||
            (err == ESP_OK && (status.on || status.fault != SY7636A_FAULT_NONE)))
            fault = true;
        failed.operation_error = err != ESP_OK ? err : ESP_ERR_INVALID_STATE;
        failed.cleanup_error = end_err;
        release(NULL);
        return failed;
    }
    ESP_LOGI(TAG, "GC16 preflight: PMU VCOM=%d mV; temperature deferred until ON=1", pmu.vcom_mv);
    read_pico_refresh_gate_t gate = {true, pmu.vcom_mv, false, 0};
    // Already locked; the core uses a scoped borrowed-lock view so no unlock/relock race.
    read_pico_refresh_ops_t locked_ops = ops;
    locked_ops.try_acquire = borrow_lock;
    locked_ops.release = keep_lock;
#if CONFIG_READ_PICO_DISPLAY_VALIDATION
    if (vendor_gray16)
        locked_ops.draw_gc16 = vendor_gray16 == 2 ? draw_vendor_page : draw_vendor_gc16;
#endif
    read_pico_refresh_result_t result =
        second_frame == NULL
            ? read_pico_refresh_gc16(&locked_ops, gate, frame, size, hl.front_fb,
                                     READ_PICO_FRAME_BYTES)
            : read_pico_refresh_gc16_pair(&locked_ops, gate, frame, size, second_frame, second_size,
                                          hl.front_fb, READ_PICO_FRAME_BYTES);
    if (result.error == READ_PICO_REFRESH_TEMPERATURE_FAILED || result.operation_error ||
        result.cleanup_error) {
        fault = true;
        reference_known = false;
        untouched_white = false;
    }
    release(NULL);
    return result;
}
read_pico_refresh_result_t read_pico_epaper_display(const uint8_t *frame, size_t size)
{
    return display_frames(frame, size, NULL, 0, false);
}
#if CONFIG_READ_PICO_DISPLAY_VALIDATION
read_pico_refresh_result_t read_pico_epaper_display_vendor_gray16(const uint8_t *frame, size_t size)
{
    return display_frames(frame, size, NULL, 0, true);
}
read_pico_refresh_result_t read_pico_epaper_display_vendor_page(const uint8_t *frame, size_t size)
{
    return display_frames(frame, size, NULL, 0, 2);
}
read_pico_refresh_result_t read_pico_epaper_display_continuous_pair(const uint8_t *white_frame,
                                                                    size_t white_size,
                                                                    const uint8_t *target_frame,
                                                                    size_t target_size)
{
    if (target_frame == NULL) {
        read_pico_refresh_result_t failed = {.error = READ_PICO_REFRESH_BAD_FRAME,
                                             .frame_error = READ_PICO_FRAME_INVALID_ARGUMENT};
        return failed;
    }
    return display_frames(white_frame, white_size, target_frame, target_size, false);
}
#endif

esp_err_t read_pico_epaper_clear_unverified(void)
{
    if (!acquire(NULL))
        return ESP_ERR_TIMEOUT;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (initialized && !fault) {
        err = hold_scan_clocks();
        if (err != ESP_OK)
            goto clear_done;
        callback_error = ESP_OK;
        reference_known = false;
        white_pending = false;
        untouched_white = false;
        epd_poweron();
        err = callback_error;
        int pg = (err == ESP_OK && rails) ? pgood() : -1;
        if (err == ESP_OK && callback_error == ESP_OK && rails && pg == 1) {
            sy7636a_status_t state;
            err = read_pico_sy_read(sy, &state);
            if (err == ESP_OK && (!state.on || !state.pg || state.fault != SY7636A_FAULT_NONE))
                err = ESP_ERR_INVALID_STATE;
            if (err == ESP_OK) {
                epd_clear();  // No success claim: this API cannot report DMA/draw failure.
                err = callback_error;
            }
        } else if (err == ESP_OK)
            err = callback_error != ESP_OK ? callback_error : ESP_ERR_INVALID_STATE;
        esp_err_t cleanup = stop_rails();
        if (err == ESP_OK)
            err = cleanup;
        cleanup = release_scan_clocks();
        if (err == ESP_OK)
            err = cleanup;
        white_pending = err == ESP_OK;
        if (err != ESP_OK)
            fault = true;
    }
clear_done:
    release(NULL);
    return err;
}
esp_err_t read_pico_epaper_confirm_white_reference(void)
{
    if (!acquire(NULL))
        return ESP_ERR_TIMEOUT;
    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (initialized && !fault && white_pending) {
        memset(hl.back_fb, 0xff, READ_PICO_FRAME_BYTES);
        reference_known = true;
        untouched_white = true;
        white_pending = false;
        err = ESP_OK;
    }
    release(NULL);
    return err;
}
esp_err_t read_pico_epaper_deinit(void)
{
    if (display_mutex == NULL)
        return ESP_OK;
    if (!acquire(NULL))
        return ESP_ERR_TIMEOUT;
    esp_err_t err = ESP_OK;
    if (lease) {
        callback_error = ESP_OK;
        err = stop_rails();
        esp_err_t unlock_err = release_scan_clocks();
        if (err == ESP_OK)
            err = unlock_err;
        if (err == ESP_OK && renderer_initialized) {
            epd_deinit();
            renderer_initialized = false;
            err = callback_error;
        }
        if (err == ESP_OK) {
            err = read_pico_sy_deinit(&sy);
            if (err == ESP_OK)
                err = delete_scan_locks();
            if (err == ESP_OK) {
                free_buffers();
                err = release_lease();
                if (err == ESP_OK) {
                    initialized = reference_known = white_pending = untouched_white = false;
                } else
                    fault = true;
            } else
                fault = true;
        }
    }
    release(NULL);
    return err;
}
bool read_pico_epaper_faulted(void)
{
    if (!acquire(NULL))
        return true;
    bool value = fault;
    release(NULL);
    return value;
}

uint16_t epaper_get_width(void)
{
    return READ_PICO_PANEL_WIDTH;
}
uint16_t epaper_get_height(void)
{
    return READ_PICO_PANEL_HEIGHT;
}
void epaper_init(const epaper_config_t *cfg)
{
    (void) cfg;
    ESP_LOGE(TAG, "Use explicit Read Pico initialization capability; legacy init rejected");
}
void epaper_display(uint8_t *image)
{
    read_pico_refresh_result_t result = read_pico_epaper_display(image, READ_PICO_FRAME_BYTES);
    if (!result.completed)
        ESP_LOGE(TAG, "Refresh failed: stage=%d operation=%d cleanup=%d", result.error,
                 result.operation_error, result.cleanup_error);
}
void epaper_clear(uint8_t *image, uint8_t color)
{
    if (image == NULL || color > 15)
        return;
    memset(image, (color << 4) | color, READ_PICO_FRAME_BYTES);
    epaper_display(image);
}
void epaper_set_temperature(int8_t value)
{
    (void) value; /* Never substitute for live SY temperature. */
}
void epaper_enter_deepsleep(void)
{
    esp_err_t err = read_pico_epaper_deinit();
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Display deinit failed: %s", esp_err_to_name(err));
}
