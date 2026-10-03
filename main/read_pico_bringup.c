#include <string.h>

#include "board_hal.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "read_pico_board.h"
#include "read_pico_epaper.h"
#include "read_pico_frame.h"

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 1, 0)
#error "Read Pico P1 diagnostic requires the locked ESP-IDF v6.1.0 baseline"
#endif

void app_main(void)
{
    const char *tag = "read_pico_bringup";
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    ESP_LOGW(tag, "P1 diagnostic only: no display/SD/NVS/network/sleep; not a release firmware");
    // Retain the native display API and its dependencies in the diagnostic link without
    // invoking it. Volatile pointer read prevents a compile-only false-positive after GC.
    read_pico_refresh_result_t (*volatile display_api)(const uint8_t *, size_t) =
        read_pico_epaper_display;
    if (display_api == NULL || read_pico_epaper_init(false) != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(tag, "Display capability guard failed; no board startup");
        return;
    }
    ESP_LOGI(tag, "Board=%s IDF=%s chip_model=%d PSRAM=%u", BOARD_HAL_NAME,
             esp_app_get_description()->idf_ver, (int) chip.model, (unsigned) esp_psram_get_size());
    // Memory-only format smoke check. This does not initialize or energize the panel.
    const uint8_t source[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    const uint8_t expected[8] = {0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe};
    uint8_t converted[8];
    const read_pico_frame_layout_t layout = {16, 1, 8};
    if (read_pico_frame_convert(source, sizeof(source), layout, converted, sizeof(converted),
                                layout, READ_PICO_ROTATE_0, false) != READ_PICO_FRAME_OK ||
        memcmp(converted, expected, sizeof(expected)) != 0) {
        ESP_LOGE(tag, "Memory-only 4bpp check failed; no board startup");
        return;
    }
    ESP_LOGI(tag, "4bpp memory-only check passed; panel remains disabled");
    esp_err_t err = board_hal_init();
    if (err != ESP_OK) {
        ESP_LOGE(tag, "Board observation failed: %s; no business startup", esp_err_to_name(err));
        return;
    }
    read_pico_board_snapshot_t state;
    err = read_pico_board_get_snapshot(&state);
    if (err == ESP_OK) {
        ESP_LOGI(tag, "PMU fw=%u.%u.%u hw=%u state=%u faults=0x%lx", state.identity.firmware[0],
                 state.identity.firmware[1], state.identity.firmware[2], state.identity.hardware,
                 state.status.power_state, (unsigned long) state.status.fault_flags);
        ESP_LOGI(tag, "Battery mV=%d percent=%d charging=%d external_power=%s",
                 state.status.battery_mv, state.status.battery_percent, state.status.charging,
                 state.status.external_power == READ_PICO_POWER_EXTERNAL ? "present" : "unknown");
        ESP_LOGI(tag, "IOE cfg0=0x%02x out0=0x%02x SY responding=%d (off-rail NACK is expected)",
                 state.ioe_config, state.ioe_output, state.sy_responding);
        ESP_LOGI(tag, "Factory VCOM valid=%d mV=%d; display remains disabled", state.vcom_valid,
                 state.vcom_mv);
    }
    esp_err_t cleanup = read_pico_board_deinit();
    if (cleanup != ESP_OK)
        ESP_LOGE(tag, "Cleanup failed: %s", esp_err_to_name(cleanup));
    ESP_LOGW(tag, "Diagnostic ended; no HOST_READY/events ACK sent; PMU boot timeout may occur");
}
