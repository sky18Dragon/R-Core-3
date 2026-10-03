#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "read_pico_pmu_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    read_pico_identity_t identity;
    read_pico_status_t status;
    int vcom_mv;
    bool vcom_valid;
    uint8_t ioe_config;
    uint8_t ioe_output;
    bool sy_responding;
} read_pico_board_snapshot_t;

// Startup init is single-task. Runtime I/O and snapshots share one recursive mutex.
// Never acquire the display lock while holding this bus lock (display -> bus order).
esp_err_t read_pico_board_lock(uint32_t timeout_ms);
void read_pico_board_unlock(void);
// Display lease prevents HAL deinit while EPDiy/SY borrow the bus. Caller holds bus lock.
esp_err_t read_pico_board_display_claim(i2c_master_bus_handle_t *out);
esp_err_t read_pico_board_display_release(void);
esp_err_t read_pico_board_ioe_read(uint8_t reg, uint8_t *value);
esp_err_t read_pico_board_ioe_configure_display(void);
esp_err_t read_pico_board_ioe_update(uint8_t mask, uint8_t value);

esp_err_t read_pico_board_refresh(void);
esp_err_t read_pico_board_get_snapshot(read_pico_board_snapshot_t *out);
esp_err_t read_pico_board_deinit(void);

#ifdef __cplusplus
}
#endif
