#pragma once

#include "sy7636a.h"

typedef struct read_pico_sy_power *read_pico_sy_handle_t;
// Private standard volatile startup sequence from the locked manufacturer. No PMU/NVS writes.
// This wrapper differs in cleanup/error propagation; calibration policy remains external.
esp_err_t read_pico_sy_init(i2c_master_bus_handle_t bus, int factory_vcom_mv, esp_err_t (*en)(bool),
                            esp_err_t (*vcom_en)(bool), int (*pgood)(void),
                            read_pico_sy_handle_t *out);
esp_err_t read_pico_sy_deinit(read_pico_sy_handle_t *handle);
esp_err_t read_pico_sy_power_on(read_pico_sy_handle_t handle);
esp_err_t read_pico_sy_power_off(read_pico_sy_handle_t handle);
esp_err_t read_pico_sy_i2c_begin(read_pico_sy_handle_t handle, bool *woke);
esp_err_t read_pico_sy_i2c_end(read_pico_sy_handle_t handle, bool woke);
esp_err_t read_pico_sy_read(read_pico_sy_handle_t handle, sy7636a_status_t *status);
int read_pico_sy_get_vcom(read_pico_sy_handle_t handle);
