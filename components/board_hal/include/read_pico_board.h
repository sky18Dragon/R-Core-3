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
// Shared I2C bus for other on-board peripherals (touch). Caller holds the
// board lock for every transaction; refused while the display owns the bus.
esp_err_t read_pico_board_i2c_bus(i2c_master_bus_handle_t *out);
// PMU device handle for the opt-in sleep session (CONFIG_READ_PICO_PMU_SLEEP);
// caller holds the board lock.
esp_err_t read_pico_board_pmu_device(i2c_master_dev_handle_t *out);
// SD owner holds a lease until successful unmount; caller holds the bus lock.
esp_err_t read_pico_board_storage_claim(void);
esp_err_t read_pico_board_storage_release(void);
// Fresh, read-only active-low P0.6 observation. Errors never imply card presence.
esp_err_t read_pico_board_sd_present(bool *present);
esp_err_t read_pico_board_ioe_read(uint8_t reg, uint8_t *value);
esp_err_t read_pico_board_ioe_configure_display(void);
esp_err_t read_pico_board_ioe_update(uint8_t mask, uint8_t value);

esp_err_t read_pico_board_refresh(void);
esp_err_t read_pico_board_get_snapshot(read_pico_board_snapshot_t *out);
esp_err_t read_pico_board_deinit(void);

#ifdef __cplusplus
}
#endif
