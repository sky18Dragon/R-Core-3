/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2026 mindreset
 * Standard volatile register sequence derived from MindReset sy7636a.c at the locked gitlink.
 * Outer adapter: preserve native failures and retryable device ownership; no calibration API.
 */
#include "read_pico_sy_power.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct read_pico_sy_power {
    i2c_master_dev_handle_t device;
    int vcom_mv;
    esp_err_t (*en)(bool);
    esp_err_t (*vcom_en)(bool);
    int (*pgood)(void);
    bool en_on;
    bool rails;
};

static esp_err_t read_register(read_pico_sy_handle_t h, uint8_t reg, uint8_t *value)
{
    // Manufacturer/datasheet require a STOP before the read, not repeated START.
    esp_err_t err = i2c_master_transmit(h->device, &reg, 1, 100);
    return err == ESP_OK ? i2c_master_receive(h->device, value, 1, 100) : err;
}
static esp_err_t write_register(read_pico_sy_handle_t h, uint8_t reg, uint8_t value)
{
    uint8_t frame[2] = {reg, value};
    return i2c_master_transmit(h->device, frame, sizeof(frame), 100);
}
static void first_error(esp_err_t *result, esp_err_t next)
{
    if (*result == ESP_OK && next != ESP_OK)
        *result = next;
}

esp_err_t read_pico_sy_init(i2c_master_bus_handle_t bus, int mv, esp_err_t (*en)(bool),
                            esp_err_t (*vcom_en)(bool), int (*pgood)(void),
                            read_pico_sy_handle_t *out)
{
    if (!bus || !out || !en || !vcom_en || !pgood || mv < 500 || mv > 2500 || mv % 10)
        return ESP_ERR_INVALID_ARG;
    *out = NULL;
    read_pico_sy_handle_t h = calloc(1, sizeof(*h));
    if (!h)
        return ESP_ERR_NO_MEM;
    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x62, .scl_speed_hz = 400000};
    esp_err_t err = i2c_master_bus_add_device(bus, &config, &h->device);
    if (err != ESP_OK) {
        free(h);
        return err;
    }
    h->vcom_mv = mv;
    h->en = en;
    h->vcom_en = vcom_en;
    h->pgood = pgood;
    *out = h;
    return ESP_OK;
}

esp_err_t read_pico_sy_i2c_begin(read_pico_sy_handle_t h, bool *woke)
{
    if (!h || !woke)
        return ESP_ERR_INVALID_ARG;
    *woke = false;
    if (h->en_on)
        return ESP_OK;
    esp_err_t err = h->en(true);
    if (err == ESP_OK) {
        h->en_on = true;
        *woke = true;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return err;
}
esp_err_t read_pico_sy_i2c_end(read_pico_sy_handle_t h, bool woke)
{
    if (!h)
        return ESP_ERR_INVALID_ARG;
    if (!woke || h->rails)
        return ESP_OK;
    esp_err_t err = h->en(false);
    if (err == ESP_OK)
        h->en_on = false;
    return err;
}

esp_err_t read_pico_sy_power_off(read_pico_sy_handle_t h)
{
    if (!h)
        return ESP_ERR_INVALID_ARG;
    esp_err_t err = h->vcom_en(false);
    if (h->en_on)
        first_error(&err, write_register(h, 0x00, 0x00));
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_err_t disable = h->en(false);
    first_error(&err, disable);
    if (disable == ESP_OK)
        h->en_on = false;
    if (err == ESP_OK)
        h->rails = false;
    return err;
}
esp_err_t read_pico_sy_power_on(read_pico_sy_handle_t h)
{
    if (!h)
        return ESP_ERR_INVALID_ARG;
    if (h->rails)
        return ESP_OK;
    bool woke;
    esp_err_t err = read_pico_sy_i2c_begin(h, &woke);
    if (err == ESP_OK)
        err = h->vcom_en(false);
    uint8_t msb = 0;
    if (err == ESP_OK)
        err = read_register(h, 2, &msb);
    int code = h->vcom_mv / 10;
    // Preserve the manufacturer's reserved bits, standard +/-15 V and 2 ms sequencing.
    if (err == ESP_OK)
        err = write_register(h, 1, (uint8_t) code);
    if (err == ESP_OK)
        err = write_register(h, 2, (msb & 0x7f) | ((code & 0x100) ? 0x80 : 0));
    if (err == ESP_OK)
        err = write_register(h, 3, 0x66);
    if (err == ESP_OK)
        err = write_register(h, 6, 0xaa);
    if (err == ESP_OK)
        err = write_register(h, 0, 0xc0);
    if (err != ESP_OK)
        return err;  // Outer transaction ALWAYS invokes power_off on this failure.
    bool good = false;
    // Bounded poll count even if an injected timer fails; each I/O has a 100 ms timeout.
    for (int attempt = 0; attempt < 30; ++attempt) {
        int pg = h->pgood();
        if (pg < 0)
            return ESP_FAIL;
        if (pg > 0) {
            good = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!good)
        return ESP_ERR_TIMEOUT;
    err = h->vcom_en(true);
    if (err == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(5));
        h->rails = true;
    }
    return err;
}

esp_err_t read_pico_sy_read(read_pico_sy_handle_t h, sy7636a_status_t *status)
{
    if (!h || !status || !h->en_on)
        return ESP_ERR_INVALID_STATE;
    memset(status, 0, sizeof(*status));
    uint8_t op, fault, temp;
    esp_err_t err = read_register(h, 0, &op);
    if (err == ESP_OK)
        err = read_register(h, 7, &fault);
    if (err == ESP_OK)
        err = read_register(h, 8, &temp);
    if (err != ESP_OK)
        return err;
    status->on = (op & 0x80) != 0;
    status->pg = (fault & 1) != 0;
    status->fault_reg = fault;
    status->fault = sy7636a_fault_from_reg(fault);
    status->temperature_c = (int8_t) temp;
    status->vcom_mv = h->vcom_mv;
    return ESP_OK;
}

esp_err_t read_pico_sy_deinit(read_pico_sy_handle_t *handle)
{
    if (!handle)
        return ESP_ERR_INVALID_ARG;
    read_pico_sy_handle_t h = *handle;
    if (!h)
        return ESP_OK;
    esp_err_t err = read_pico_sy_power_off(h);
    if (err == ESP_OK)
        err = i2c_master_bus_rm_device(h->device);
    if (err != ESP_OK)
        return err;  // Keep valid handle for cleanup retry.
    free(h);
    *handle = NULL;
    return ESP_OK;
}
int read_pico_sy_get_vcom(read_pico_sy_handle_t h)
{
    return h ? h->vcom_mv : -1;
}
