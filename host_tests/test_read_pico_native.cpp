#include <gtest/gtest.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" {
#include "driver/gpio.h"
#include "e0470_epaper_waveform.h"
#include "esp_clk_tree.h"
#include "esp_heap_caps.h"
#include "esp_pm.h"
#include "freertos/semphr.h"
#include "read_pico_board.h"
#include "read_pico_epaper.h"
#include "read_pico_epd_timing.h"
#include "read_pico_validation.h"
}

namespace
{
bool display_lock, bus_lock, leased, pg_stuck, pg_read_error, callback_write_error;
int devices, allocations, fail_alloc_at = -1, fail_draw_at = -1, draw_calls, clear_calls, lcd_calls,
                          vcom = 1250;
int claim_calls, mutex_token, bus_token, device_token;
int pm_locks, pm_acquired;
int from_white_calls, vendor_rect_calls;
bool vendor_render_corrupt;
int fail_sy_reg = -1;
int fault_code, rail_starts;
int active_temperature = 24, active_temp_reads, fail_active_temp_read = -1;
bool fail_active_sample_pgood;
std::vector<bool> temperature_read_on;
uint8_t output = 1, pointer_reg;
std::array<uint8_t, 9> regs = {0, 0, 0x14, 0, 0, 0, 0, 0, 24};
const EpdBoardDefinition *board;
epd_ctrl_state_t ctrl;
EpdDrawError draw_error = EPD_DRAW_SUCCESS;
esp_err_t pmu_error = ESP_OK, configure_error = ESP_OK, remove_error = ESP_OK;
size_t largest_block = 100000;
bool reenter, reenter_busy;
std::vector<uint8_t> source(READ_PICO_FRAME_BYTES, 0x12), drawn;
int pg_level()
{
    return pg_stuck || ((regs[0] & 0x80) && (output & 8));
}
void start()
{
    ASSERT_EQ(read_pico_epaper_init(true), ESP_OK);
    EXPECT_EQ(lcd_calls, 1);
    EXPECT_TRUE(leased);
    EXPECT_EQ(pm_locks, 2);
    EXPECT_EQ(pm_acquired, 0);
}
void white()
{
    ASSERT_EQ(read_pico_epaper_clear_unverified(), ESP_OK);
    ASSERT_EQ(read_pico_epaper_confirm_white_reference(), ESP_OK);
}
int session_clear(void *)
{
    esp_err_t err = read_pico_epaper_init(true);
    return err == ESP_OK ? read_pico_epaper_clear_unverified() : err;
}
int session_confirm(void *)
{
    return read_pico_epaper_confirm_white_reference();
}
int session_stop(void *)
{
    return read_pico_epaper_deinit();
}
int session_draw(void *, read_pico_pattern_t pattern)
{
    if (!read_pico_pattern_render(pattern, source.data(), source.size()))
        return ESP_ERR_INVALID_ARG;
    auto r = pattern == READ_PICO_PATTERN_GRAY16_PAGE
                 ? read_pico_epaper_display_vendor_page(source.data(), source.size())
             : pattern == READ_PICO_PATTERN_GRAY16_VENDOR
                 ? read_pico_epaper_display_vendor_gray16(source.data(), source.size())
                 : read_pico_epaper_display(source.data(), source.size());
    return r.completed ? ESP_OK : ESP_FAIL;
}
read_pico_validation_session_t validation_session()
{
    read_pico_validation_ops_t ops{nullptr, session_clear, session_confirm, session_stop,
                                   session_draw};
    read_pico_validation_session_t session{};
    EXPECT_TRUE(read_pico_validation_init(&session, &ops));
    return session;
}
// Faults deliberately latch for the entire boot. Each assertion block uses an isolated
// child as a simulated fresh boot, including when the whole test binary is run at once.
#define FRESH_BOOT(...)                                 \
    ASSERT_EXIT(                                        \
        {                                               \
            __VA_ARGS__;                                \
            _exit(testing::Test::HasFailure() ? 1 : 0); \
        },                                              \
        testing::ExitedWithCode(0), ".*")
}  // namespace

extern "C" {
struct fake_esp_pm_lock {
    esp_pm_lock_type_t type;
    bool acquired;
};

esp_err_t esp_clk_tree_src_get_freq_hz(soc_module_clk_t source,
                                       esp_clk_tree_src_freq_precision_t, uint32_t *frequency_hz)
{
    if (frequency_hz == nullptr)
        return ESP_ERR_INVALID_ARG;
    if (source == SOC_MOD_CLK_CPU)
        *frequency_hz = pm_acquired >= 1 ? 240000000 : 40000000;
    else if (source == SOC_MOD_CLK_APB)
        *frequency_hz = pm_acquired >= 1 ? 80000000 : 40000000;
    else
        return ESP_ERR_INVALID_ARG;
    return ESP_OK;
}

esp_err_t esp_pm_lock_create(esp_pm_lock_type_t type, int, const char *, esp_pm_lock_handle_t *out)
{
    auto *lock = static_cast<fake_esp_pm_lock *>(calloc(1, sizeof(fake_esp_pm_lock)));
    if (lock == nullptr)
        return ESP_ERR_NO_MEM;
    lock->type = type;
    *out = lock;
    ++pm_locks;
    return ESP_OK;
}
esp_err_t esp_pm_lock_acquire(esp_pm_lock_handle_t handle)
{
    if (handle == nullptr || handle->acquired)
        return ESP_ERR_INVALID_STATE;
    handle->acquired = true;
    ++pm_acquired;
    return ESP_OK;
}
esp_err_t esp_pm_lock_release(esp_pm_lock_handle_t handle)
{
    if (handle == nullptr || !handle->acquired)
        return ESP_ERR_INVALID_STATE;
    handle->acquired = false;
    --pm_acquired;
    return ESP_OK;
}
esp_err_t esp_pm_lock_delete(esp_pm_lock_handle_t handle)
{
    if (handle == nullptr || handle->acquired)
        return ESP_ERR_INVALID_STATE;
    free(handle);
    --pm_locks;
    return ESP_OK;
}

const EpdWaveform E0470_WAVEFORM = {};
const EpdDisplay_t E0470_DISPLAY = {};
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *)
{
    return &mutex_token;
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t)
{
    if (display_lock)
        return pdFALSE;
    display_lock = true;
    return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t)
{
    EXPECT_TRUE(display_lock);
    display_lock = false;
    return pdTRUE;
}
uint32_t last_bus_lock_timeout;
esp_err_t read_pico_board_lock(uint32_t timeout_ms)
{
    last_bus_lock_timeout = timeout_ms;
    if (bus_lock)
        return ESP_ERR_TIMEOUT;
    bus_lock = true;
    return ESP_OK;
}
void read_pico_board_unlock(void)
{
    EXPECT_TRUE(bus_lock);
    bus_lock = false;
}
esp_err_t read_pico_board_refresh(void)
{
    EXPECT_TRUE(bus_lock);
    return pmu_error;
}
esp_err_t read_pico_board_get_snapshot(read_pico_board_snapshot_t *out)
{
    EXPECT_TRUE(bus_lock);
    memset(out, 0, sizeof(*out));
    out->vcom_valid = vcom >= 500 && vcom <= 2500;
    out->vcom_mv = vcom;
    return ESP_OK;
}
esp_err_t read_pico_board_display_claim(i2c_master_bus_handle_t *out)
{
    EXPECT_TRUE(bus_lock);
    ++claim_calls;
    leased = true;
    *out = reinterpret_cast<i2c_master_bus_handle_t>(&bus_token);
    return ESP_OK;
}
esp_err_t read_pico_board_display_release(void)
{
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(leased);
    EXPECT_EQ(devices, 0);
    leased = false;
    return ESP_OK;
}
esp_err_t read_pico_board_ioe_configure_display(void)
{
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(leased);
    return configure_error;
}
esp_err_t read_pico_board_ioe_update(uint8_t mask, uint8_t value)
{
    EXPECT_TRUE(display_lock);
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(leased);
    if (callback_write_error)
        return ESP_ERR_INVALID_RESPONSE;
    output = (output & ~mask) | value;
    if (!(output & 8)) {
        regs[0] = 0;
        regs[8] = 0;  // EN low clears thermistor readout, not a real 0 C sample.
    }
    return ESP_OK;
}
esp_err_t read_pico_board_ioe_read(uint8_t reg, uint8_t *value)
{
    EXPECT_TRUE(display_lock);
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(leased);
    if (reg == 0 && pg_read_error)
        return ESP_ERR_INVALID_RESPONSE;
    *value = reg == 2 ? output : pg_level() ? 0x20 : 0;
    return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t, const i2c_device_config_t *cfg,
                                    i2c_master_dev_handle_t *out)
{
    EXPECT_TRUE(leased);
    EXPECT_EQ(cfg->device_address, 0x62);
    ++devices;
    *out = reinterpret_cast<i2c_master_dev_handle_t>(&device_token);
    return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t)
{
    if (remove_error != ESP_OK)
        return remove_error;
    --devices;
    return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const uint8_t *data, size_t size, int)
{
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(output & 8);
    if (size == 1)
        pointer_reg = *data;
    else if (size == 2) {
        if (data[0] == fail_sy_reg)
            return ESP_ERR_TIMEOUT;
        if (data[0] == 0 && data[1] == 0xc0)
            ++rail_starts;
        regs[data[0]] = data[1];
    } else
        return ESP_FAIL;
    return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t, uint8_t *out, size_t size, int)
{
    EXPECT_EQ(size, 1U);
    if (pointer_reg == 8) {
        bool on = (regs[0] & 0x80) != 0;
        temperature_read_on.push_back(on);
        if (on) {
            if (++active_temp_reads == fail_active_temp_read)
                return ESP_ERR_TIMEOUT;
            if (active_temp_reads == 2 && fail_active_sample_pgood)
                pg_read_error = true;
            regs[8] = static_cast<uint8_t>(active_temperature);
        }
    }
    *out =
        pointer_reg == 7 ? static_cast<uint8_t>((fault_code << 1) | pg_level()) : regs[pointer_reg];
    return ESP_OK;
}
void *heap_caps_aligned_alloc(size_t alignment, size_t size, unsigned)
{
    if (allocations == fail_alloc_at)
        return nullptr;
    void *ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size))
        return nullptr;
    ++allocations;
    return ptr;
}
void *heap_caps_calloc(size_t n, size_t size, unsigned)
{
    if (allocations == fail_alloc_at)
        return nullptr;
    void *ptr = calloc(n, size);
    if (ptr)
        ++allocations;
    return ptr;
}
void heap_caps_free(void *ptr)
{
    if (ptr)
        --allocations;
    free(ptr);
}
size_t heap_caps_get_largest_free_block(unsigned)
{
    return largest_block;
}
void e0470_waveform_init(void) {}
void read_pico_epd_scan(int clock, int mode, read_pico_epd_scan_t *scan)
{
    EXPECT_EQ(clock, 18);
    EXPECT_EQ(mode, READ_PICO_EPD_SCAN_FULL);
    scan->pclk_mhz = clock;
}
void read_pico_epd_init_lcd(const lcd_bus_config_t *pins, int bits, int width, int height,
                            const read_pico_epd_scan_t *scan)
{
    EXPECT_EQ(bits, 16);
    EXPECT_EQ(width, 1216);
    EXPECT_EQ(height, 684);
    EXPECT_EQ(scan->pclk_mhz, 18);
    EXPECT_EQ(pins->clock, 21);
    ++lcd_calls;
}
void read_pico_epd_set_pclk(int clock)
{
    EXPECT_EQ(clock, 18);
}
void epd_lcd_deinit(void) {}
void epd_init(const EpdBoardDefinition *definition, const EpdDisplay_t *, int lut)
{
    EXPECT_EQ(lut, EPD_LUT_1K);
    board = definition;
    board->init(1216);
}
void epd_deinit(void)
{
    board->poweroff(&ctrl);
    board->deinit();
}
static int current_rotation;
void epd_set_rotation(int rotation)
{
    current_rotation = rotation;
}
int epd_get_rotation(void)
{
    return current_rotation;
}
int epd_width(void)
{
    return 1216;
}
int epd_height(void)
{
    return 684;
}
void epd_fill_rect(EpdRect r, uint8_t color, uint8_t *fb)
{
    ++vendor_rect_calls;
    // Boundary fake only; the separate ReadPicoVendor target compiles real epdiy.c.
    for (int y = r.y; y < r.y + r.height; ++y)
        for (int x = r.x; x < r.x + r.width; ++x) {
            int px = current_rotation == EPD_ROT_INVERTED_PORTRAIT ? y : x;
            int py = current_rotation == EPD_ROT_INVERTED_PORTRAIT ? 683 - x : y;
            auto &b = fb[py * 608 + px / 2];
            b = px & 1 ? (b & 15) | (color & 0xf0) : (b & 0xf0) | (color >> 4);
        }
    if (vendor_render_corrupt)
        fb[0] = 0x10;
}
void epd_set_leading_skip(bool skip)
{
    EXPECT_FALSE(skip);
}
void epd_poweron(void)
{
    board->poweron(&ctrl);
}
void epd_clear(void)
{
    EXPECT_EQ(pm_acquired, 2);
    ++clear_calls;
}
EpdDrawError epd_hl_update_screen_full(EpdiyHighlevelState *hl, int mode, int temperature)
{
    EXPECT_EQ(mode, MODE_GC16);
    EXPECT_EQ(temperature, active_temperature);
    EXPECT_TRUE(display_lock);
    EXPECT_TRUE(bus_lock);
    EXPECT_TRUE(output & 0x10);
    EXPECT_EQ(pm_acquired, 2);
    ++draw_calls;
    drawn.assign(hl->front_fb, hl->front_fb + READ_PICO_FRAME_BYTES);
    // Match the real vendor API's problematic back-buffer mutation even on a failure.
    memcpy(hl->back_fb, hl->front_fb, READ_PICO_FRAME_BYTES);
    if (reenter)
        reenter_busy =
            read_pico_epaper_display(source.data(), source.size()).error == READ_PICO_REFRESH_BUSY;
    return draw_calls == fail_draw_at ? MOCK_DRAW_FAILURE : draw_error;
}
EpdDrawError epd_hl_update_screen_from_white(EpdiyHighlevelState *hl, int mode, int temperature)
{
    ++from_white_calls;
    for (size_t i = 0; i < READ_PICO_FRAME_BYTES; ++i)
        EXPECT_EQ(hl->back_fb[i], 0xff);
    memset(hl->back_fb, 0xff, READ_PICO_FRAME_BYTES);
    return epd_hl_update_screen_full(hl, mode, temperature);
}
void esp_rom_gpio_connect_out_signal(int, int, bool, bool) {}
esp_err_t gpio_set_level(gpio_num_t, uint32_t)
{
    ADD_FAILURE();
    return ESP_FAIL;
}
int gpio_get_level(gpio_num_t)
{
    ADD_FAILURE();
    return -1;
}
esp_err_t gpio_config(const gpio_config_t *)
{
    ADD_FAILURE();
    return ESP_FAIL;
}
void esp_rom_delay_us(uint32_t) {}
int64_t esp_timer_get_time(void)
{
    return 0;
}
const char *esp_err_to_name(esp_err_t)
{
    return "injected";
}
}

TEST(ReadPicoNative, FalseCapabilityHasNoBoardOrPowerSideEffects)
{
    FRESH_BOOT(EXPECT_EQ(read_pico_epaper_init(false), ESP_ERR_NOT_SUPPORTED);
               EXPECT_EQ(claim_calls, 0); EXPECT_EQ(devices, 0); EXPECT_EQ(lcd_calls, 0);
               EXPECT_FALSE(display_lock); EXPECT_FALSE(bus_lock));
}
TEST(ReadPicoNative, InvalidPmuOrFactoryVcomCannotClaimDisplay)
{
    FRESH_BOOT(pmu_error = ESP_FAIL; EXPECT_EQ(read_pico_epaper_init(true), ESP_FAIL);
               pmu_error = ESP_OK; vcom = 0;
               EXPECT_EQ(read_pico_epaper_init(true), ESP_ERR_INVALID_STATE);
               EXPECT_EQ(claim_calls, 0); EXPECT_EQ(devices, 0));
}
TEST(ReadPicoNative, PreexistingRailsRefusalDoesNotCommandPowerOff)
{
    FRESH_BOOT(configure_error = ESP_ERR_INVALID_STATE; output = 0x19;
               EXPECT_EQ(read_pico_epaper_init(true), ESP_ERR_INVALID_STATE);
               EXPECT_EQ(output, 0x19); EXPECT_FALSE(leased); EXPECT_EQ(devices, 0));
}
TEST(ReadPicoNative, AllocationAndHeadroomFailuresFreeBuffersAndLease)
{
    FRESH_BOOT(fail_alloc_at = 2; EXPECT_EQ(read_pico_epaper_init(true), ESP_ERR_NO_MEM);
               EXPECT_EQ(allocations, 0); EXPECT_EQ(devices, 0); EXPECT_FALSE(leased);
               fail_alloc_at = -1; largest_block = 65535;
               EXPECT_EQ(read_pico_epaper_init(true), ESP_ERR_NO_MEM); EXPECT_EQ(allocations, 0);
               EXPECT_EQ(devices, 0); EXPECT_FALSE(leased));
}
TEST(ReadPicoNative, RefreshWaitsBrieflyForBoardLockInsteadOfFailingBusy)
{
    // Touch polling and SD checks hold the board lock for ~1 ms; a 0 ms try
    // failed 8/100 refreshes on the device.
    FRESH_BOOT(start(); white(); last_bus_lock_timeout = 0;
               EXPECT_TRUE(read_pico_epaper_display(source.data(), source.size()).completed);
               EXPECT_EQ(last_bus_lock_timeout, (uint32_t) READ_PICO_EPAPER_BUS_WAIT_MS);
               EXPECT_GE(READ_PICO_EPAPER_BUS_WAIT_MS, 100);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, FirstFrameRequiresExplicitWhiteReferenceAndReentryIsBusy)
{
    FRESH_BOOT(start(); EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                                  READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(draw_calls, 0);
               EXPECT_EQ(read_pico_epaper_confirm_white_reference(), ESP_ERR_INVALID_STATE);
               white(); reenter = true;
               EXPECT_TRUE(read_pico_epaper_display(source.data(), source.size()).completed);
               EXPECT_TRUE(reenter_busy); EXPECT_EQ(draw_calls, 1);
               ASSERT_EQ(drawn.size(), READ_PICO_FRAME_BYTES); EXPECT_EQ(drawn.front(), 0x21);
               EXPECT_EQ(drawn.back(), 0x21); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK); EXPECT_EQ(allocations, 0);
               EXPECT_EQ(devices, 0); EXPECT_FALSE(leased));
}

TEST(ReadPicoNative, ScanClockLocksCoverClearAndGc16AndAreReleasedAfterEachTransaction)
{
    FRESH_BOOT(start(); EXPECT_EQ(pm_locks, 2); EXPECT_EQ(pm_acquired, 0);
               ASSERT_EQ(read_pico_epaper_clear_unverified(), ESP_OK);
               EXPECT_EQ(clear_calls, 1); EXPECT_EQ(pm_acquired, 0);
               ASSERT_EQ(read_pico_epaper_confirm_white_reference(), ESP_OK);
               auto result = read_pico_epaper_display(source.data(), source.size());
               EXPECT_TRUE(result.completed); EXPECT_EQ(draw_calls, 1); EXPECT_EQ(pm_acquired, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK);
               EXPECT_EQ(pm_acquired, 0); EXPECT_EQ(pm_locks, 0));
}
TEST(ReadPicoNative, MalformedFrameAndUnknownTemperatureNeverDraw)
{
    FRESH_BOOT(
        start(); white(); EXPECT_EQ(read_pico_epaper_display(nullptr, source.size()).error,
                                    READ_PICO_REFRESH_BAD_FRAME);
        EXPECT_EQ(read_pico_epaper_display(source.data(), 1).error, READ_PICO_REFRESH_BAD_FRAME);
        active_temperature = -1;
        EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                  READ_PICO_REFRESH_TEMPERATURE_FAILED);
        EXPECT_EQ(draw_calls, 0); EXPECT_EQ(output & 0x18, 0);
        EXPECT_TRUE(read_pico_epaper_faulted()); EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
    FRESH_BOOT(start(); white(); fault_code = 15; int before = rail_starts;
               EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(rail_starts, before); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, StandbyResetTemperatureIsNotPassedToRenderer)
{
    FRESH_BOOT(start(); white(); temperature_read_on.clear(); active_temp_reads = 0;
               EXPECT_EQ(regs[8], 0); active_temperature = 31;
               EXPECT_TRUE(read_pico_epaper_display(source.data(), source.size()).completed);
               EXPECT_EQ(temperature_read_on, (std::vector<bool>{false, true, true}));
               EXPECT_EQ(draw_calls, 1); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ContinuousWhiteThenTargetUsesOneRailIntervalAndOneTemperatureSample)
{
    FRESH_BOOT(
        start(); white(); temperature_read_on.clear(); active_temp_reads = 0;
        std::vector<uint8_t> white_frame(READ_PICO_FRAME_BYTES, 0xff); int starts = rail_starts;
        auto r = read_pico_epaper_display_continuous_pair(white_frame.data(), white_frame.size(),
                                                          source.data(), source.size());
        EXPECT_TRUE(r.completed); EXPECT_EQ(r.frames_completed, 2u); EXPECT_EQ(draw_calls, 2);
        EXPECT_EQ(rail_starts, starts + 1); EXPECT_EQ(active_temp_reads, 2);
        EXPECT_EQ(temperature_read_on, (std::vector<bool>{false, true, true}));
        ASSERT_EQ(drawn.size(), READ_PICO_FRAME_BYTES); EXPECT_EQ(drawn.front(), 0x21);
        EXPECT_EQ(drawn.back(), 0x21); EXPECT_EQ(output & 0x1a, 0);
        EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ContinuousSecondDrawFailureStopsAndLatchesUnknownReference)
{
    FRESH_BOOT(start(); white(); std::vector<uint8_t> white_frame(READ_PICO_FRAME_BYTES, 0xff);
               fail_draw_at = 2;
               auto r = read_pico_epaper_display_continuous_pair(
                   white_frame.data(), white_frame.size(), source.data(), source.size());
               EXPECT_FALSE(r.completed); EXPECT_EQ(r.frames_completed, 1u);
               EXPECT_EQ(r.error, READ_PICO_REFRESH_DRAW_FAILED);
               EXPECT_EQ(r.operation_error, MOCK_DRAW_FAILURE); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ContinuousMissingSecondFrameNeverStartsRails)
{
    FRESH_BOOT(start(); white(); std::vector<uint8_t> white_frame(READ_PICO_FRAME_BYTES, 0xff);
               int starts = rail_starts; auto r = read_pico_epaper_display_continuous_pair(
                                             white_frame.data(), white_frame.size(), nullptr, 0);
               EXPECT_FALSE(r.completed); EXPECT_EQ(r.error, READ_PICO_REFRESH_BAD_FRAME);
               EXPECT_EQ(r.frame_error, READ_PICO_FRAME_INVALID_ARGUMENT);
               EXPECT_EQ(rail_starts, starts); EXPECT_EQ(draw_calls, 0);
               EXPECT_FALSE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ActiveTemperatureIoFailureStopsAndLatchesBeforeDraw)
{
    FRESH_BOOT(start(); white(); active_temp_reads = 0; fail_active_temp_read = 2;
               auto r = read_pico_epaper_display(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
               EXPECT_EQ(r.operation_error, ESP_ERR_TIMEOUT); EXPECT_EQ(r.cleanup_error, 0);
               EXPECT_FALSE(r.completed); EXPECT_EQ(draw_calls, 0); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ActiveTemperatureOverFiftyStopsWithoutRendering)
{
    FRESH_BOOT(start(); white(); active_temperature = 51;
               auto r = read_pico_epaper_display(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED); EXPECT_FALSE(r.completed);
               EXPECT_EQ(draw_calls, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(output & 0x1a, 0); EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ActiveSamplePgoodIoErrorRetainsPrimaryAndCleanupFailures)
{
    FRESH_BOOT(start(); white(); active_temp_reads = 0; fail_active_sample_pgood = true;
               auto r = read_pico_epaper_display(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
               EXPECT_EQ(r.operation_error, ESP_ERR_INVALID_RESPONSE);
               EXPECT_EQ(r.cleanup_error, ESP_ERR_INVALID_RESPONSE); EXPECT_FALSE(r.completed);
               EXPECT_EQ(draw_calls, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(output & 0x1a, 0); pg_read_error = false;
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, ActiveZeroIsValidOnlyWhenObtainedAfterPowerOn)
{
    FRESH_BOOT(start(); white(); active_temperature = 0;
               EXPECT_TRUE(read_pico_epaper_display(source.data(), source.size()).completed);
               EXPECT_EQ(draw_calls, 1); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
TEST(ReadPicoNative, ChangedVcomIsRejectedWithoutReconfiguringPower)
{
    FRESH_BOOT(start(); white(); vcom = 1300;
               EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(draw_calls, 0); EXPECT_EQ(output & 0x18, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
TEST(ReadPicoNative, PgoodReadErrorCannotEnterVoidClearOrConfirmWhite)
{
    FRESH_BOOT(start(); pg_read_error = true;
               EXPECT_NE(read_pico_epaper_clear_unverified(), ESP_OK); EXPECT_EQ(clear_calls, 0);
               EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_NE(read_pico_epaper_confirm_white_reference(), ESP_OK); pg_read_error = false;
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
TEST(ReadPicoNative, FailedDrawLatchesUnknownPhysicalReferenceForThisBoot)
{
    FRESH_BOOT(start(); white(); draw_error = MOCK_DRAW_FAILURE;
               auto result = read_pico_epaper_display(source.data(), source.size());
               EXPECT_FALSE(result.completed);
               EXPECT_EQ(result.error, READ_PICO_REFRESH_DRAW_FAILED);
               EXPECT_EQ(result.operation_error, MOCK_DRAW_FAILURE);
               EXPECT_EQ(result.cleanup_error, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_display(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_NE(read_pico_epaper_confirm_white_reference(), ESP_OK);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK);
               EXPECT_EQ(read_pico_epaper_init(true), ESP_ERR_INVALID_STATE));
}
TEST(ReadPicoNative, CallbackWriteErrorDuringDrawPreservedAcrossStopRails)
{
    FRESH_BOOT(start(); white();
               fail_draw_at = 1;
               // When draw runs, simulate a failure in poweroff/ctrl
               // epd_hl_update_screen_full mock will set callback_write_error during draw
               draw_error = MOCK_DRAW_FAILURE;
               auto result = read_pico_epaper_display(source.data(), source.size());
               EXPECT_FALSE(result.completed);
               EXPECT_EQ(result.error, READ_PICO_REFRESH_DRAW_FAILED);
               EXPECT_EQ(result.operation_error, MOCK_DRAW_FAILURE);
               EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
TEST(ReadPicoNative, PartialPrepareFailureIsStoppedAndLatchedWithoutDrawing)
{
    FRESH_BOOT(start(); white(); fail_sy_reg = 6;
               auto result = read_pico_epaper_display(source.data(), source.size());
               EXPECT_EQ(result.error, READ_PICO_REFRESH_PREPARE_FAILED);
               EXPECT_EQ(result.operation_error, ESP_ERR_TIMEOUT);
               EXPECT_EQ(result.cleanup_error, 0); EXPECT_EQ(draw_calls, 0);
               EXPECT_EQ(output & 0x1a, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
TEST(ReadPicoNative, StuckPgoodPreventsSuccessAndRetainsLeaseUntilCleanupRetry)
{
    FRESH_BOOT(start(); white(); pg_stuck = true;
               auto result = read_pico_epaper_display(source.data(), source.size());
               EXPECT_FALSE(result.completed);
               EXPECT_EQ(result.error, READ_PICO_REFRESH_STOP_FAILED);
               EXPECT_NE(result.cleanup_error, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_NE(read_pico_epaper_deinit(), ESP_OK); EXPECT_TRUE(leased); pg_stuck = false;
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK); EXPECT_FALSE(leased);
               EXPECT_EQ(devices, 0); EXPECT_EQ(allocations, 0));
}
TEST(ReadPicoNative, RemovalFailureRetainsDeviceAndBusLeaseForRetry)
{
    FRESH_BOOT(start(); remove_error = ESP_FAIL; EXPECT_EQ(read_pico_epaper_deinit(), ESP_FAIL);
               EXPECT_TRUE(leased); EXPECT_EQ(devices, 1); remove_error = ESP_OK;
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK); EXPECT_FALSE(leased);
               EXPECT_EQ(devices, 0); EXPECT_EQ(allocations, 0));
}

TEST(ReadPicoNative, PatternSessionUsesRealConversionAndGc16WithManualGates)
{
    FRESH_BOOT(
        auto s = validation_session();
        EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_PARITY));
        EXPECT_EQ(draw_calls, 0);
        ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
        ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
        for (int p = 0; p < READ_PICO_PATTERN_GRAY16_VENDOR; ++p) {
            auto c =
                static_cast<read_pico_validation_command_t>(READ_PICO_VALIDATION_DRAW_WHITE + p);
            ASSERT_TRUE(read_pico_validation_apply(&s, c));
            EXPECT_EQ(draw_calls, p + 1);
            EXPECT_EQ(output & 0x1a, 0);
            EXPECT_EQ(s.confirmed_mask, (1u << p) - 1);
            ASSERT_EQ(drawn.size(), READ_PICO_FRAME_BYTES);
            for (size_t i = 0; i < source.size(); ++i)
                ASSERT_EQ(drawn[i], static_cast<uint8_t>((source[i] << 4) | (source[i] >> 4)));
            EXPECT_FALSE(read_pico_validation_apply(&s, c));
            ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
        } EXPECT_EQ(s.confirmed_mask, 0x7fu);
        EXPECT_EQ(clear_calls, 1);
        ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
        EXPECT_EQ(allocations, 0); EXPECT_EQ(devices, 0); EXPECT_FALSE(leased));
}

TEST(ReadPicoNative, PatternDrawFailureLatchesBothSessionAndNativeReference)
{
    FRESH_BOOT(auto s = validation_session();
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
               draw_error = MOCK_DRAW_FAILURE;
               EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
               EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
               EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
               EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_CORNERS));
               EXPECT_EQ(draw_calls, 1); EXPECT_EQ(s.confirmed_mask | s.drawn_mask, 0u);
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
               EXPECT_EQ(allocations, 0); EXPECT_EQ(devices, 0); EXPECT_FALSE(leased));
}

TEST(ReadPicoNative, VendorControlUsesFreshWhiteRealTransactionAndFromWhiteBoundary)
{
    FRESH_BOOT(auto s = validation_session();
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR));
               EXPECT_EQ(vendor_rect_calls, 16); EXPECT_EQ(from_white_calls, 1);
               EXPECT_EQ(draw_calls, 1); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(s.drawn_mask, 0x80u); EXPECT_EQ(s.confirmed_mask, 0u);
               for (size_t i = 0; i < source.size(); ++i)
                   ASSERT_EQ(drawn[i], static_cast<uint8_t>((source[i] << 4) | (source[i] >> 4)));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
               EXPECT_EQ(allocations, 0); EXPECT_EQ(devices, 0));
}

TEST(ReadPicoNative, OfficialPageUsesFromWhiteAndCannotReuseItsReference)
{
    FRESH_BOOT(auto s = validation_session();
               EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_PAGE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_PAGE));
               EXPECT_EQ(from_white_calls, 1); EXPECT_EQ(draw_calls, 1);
               EXPECT_EQ(s.drawn_mask, 0x100u); EXPECT_EQ(s.confirmed_mask, 0u);
               EXPECT_EQ(epd_get_rotation(), EPD_ROT_LANDSCAPE); int starts = rail_starts;
               EXPECT_EQ(read_pico_epaper_display_vendor_page(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(starts, rail_starts);
               ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
               EXPECT_EQ(allocations, 0); EXPECT_EQ(devices, 0));
}

TEST(ReadPicoNative, OfficialPageMismatchDoesNotDrawAndRestoresRotation)
{
    FRESH_BOOT(start(); white(); ASSERT_TRUE(read_pico_pattern_render(
        READ_PICO_PATTERN_GRAY16_PAGE, source.data(), source.size()));
               vendor_render_corrupt = true;
               auto r = read_pico_epaper_display_vendor_page(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_DRAW_FAILED); EXPECT_EQ(draw_calls, 0);
               EXPECT_EQ(epd_get_rotation(), EPD_ROT_LANDSCAPE);
               EXPECT_TRUE(read_pico_epaper_faulted()); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, VendorControlCannotBorrowNonWhiteReferenceEvenOutsideSession)
{
    FRESH_BOOT(start(); white();
               ASSERT_TRUE(read_pico_epaper_display(source.data(), source.size()).completed);
               int starts = rail_starts;
               auto r = read_pico_epaper_display_vendor_gray16(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_GATE_CLOSED); EXPECT_EQ(rail_starts, starts);
               EXPECT_EQ(from_white_calls, 0); EXPECT_EQ(vendor_rect_calls, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, VendorControlMismatchIsStoppedAndFaultLatchedBeforeHlDraw)
{
    FRESH_BOOT(start(); white(); ASSERT_TRUE(read_pico_pattern_render(
        READ_PICO_PATTERN_GRAY16_VENDOR, source.data(), source.size()));
               vendor_render_corrupt = true;
               auto r = read_pico_epaper_display_vendor_gray16(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_DRAW_FAILED); EXPECT_FALSE(r.completed);
               EXPECT_EQ(from_white_calls, 0); EXPECT_EQ(draw_calls, 0);
               EXPECT_EQ(output & 0x1a, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, VendorControlTemperatureFailureNeverCallsVendorDrawing)
{
    FRESH_BOOT(start(); white(); active_temperature = 51;
               auto r = read_pico_epaper_display_vendor_gray16(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
               EXPECT_EQ(vendor_rect_calls, 0); EXPECT_EQ(from_white_calls, 0);
               EXPECT_EQ(draw_calls, 0); EXPECT_EQ(output & 0x1a, 0);
               EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}

TEST(ReadPicoNative, VendorHlFailurePreservesFailureAndCannotBeRetried)
{
    FRESH_BOOT(start(); white(); ASSERT_TRUE(read_pico_pattern_render(
        READ_PICO_PATTERN_GRAY16_VENDOR, source.data(), source.size()));
               draw_error = MOCK_DRAW_FAILURE;
               auto r = read_pico_epaper_display_vendor_gray16(source.data(), source.size());
               EXPECT_EQ(r.error, READ_PICO_REFRESH_DRAW_FAILED);
               EXPECT_EQ(r.operation_error, MOCK_DRAW_FAILURE); EXPECT_EQ(from_white_calls, 1);
               EXPECT_EQ(output & 0x1a, 0); EXPECT_TRUE(read_pico_epaper_faulted());
               EXPECT_EQ(read_pico_epaper_display_vendor_gray16(source.data(), source.size()).error,
                         READ_PICO_REFRESH_GATE_CLOSED);
               EXPECT_EQ(from_white_calls, 1); EXPECT_EQ(read_pico_epaper_deinit(), ESP_OK));
}
