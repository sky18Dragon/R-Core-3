#include "read_pico_accel.h"

#include <string.h>

#include "esp_log.h"
#include "read_pico_board.h"
#include "sc7a20h.h"

#define READ_PICO_ACCEL_INIT_LOCK_MS 100

static const char *TAG = "read_pico_accel";
static sc7a20h_handle_t s_accel;
static bool s_failed;

esp_err_t read_pico_accel_start(void)
{
    if (s_accel)
        return ESP_OK;
    if (s_failed)
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = read_pico_board_lock(READ_PICO_ACCEL_INIT_LOCK_MS);
    if (err != ESP_OK)
        return err;  // busy: the caller may try again later
    i2c_master_bus_handle_t bus = NULL;
    err = read_pico_board_i2c_bus(&bus);
    if (err == ESP_OK) {
        sc7a20h_config_t config = SC7A20H_DEVICE_CONFIG_DEFAULT();  // INT1 not connected
        err = sc7a20h_init(bus, &config, &s_accel);
    }
    read_pico_board_unlock();
    if (err != ESP_OK) {
        s_accel = NULL;
        s_failed = true;
        ESP_LOGW(TAG, "Accelerometer unavailable (%s); orientation fixed until restart",
                 esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Accelerometer ready");
    return ESP_OK;
}

bool read_pico_accel_ready(void)
{
    return s_accel != NULL;
}

esp_err_t read_pico_accel_read(read_pico_accel_sample_t *out)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    if (s_accel == NULL)
        return ESP_ERR_INVALID_STATE;
    if (read_pico_board_lock(0) != ESP_OK)
        return ESP_ERR_TIMEOUT;
    sc7a20h_sample_t sample;
    esp_err_t err = sc7a20h_read(s_accel, &sample);
    read_pico_board_unlock();
    if (err != ESP_OK)
        return err;
    out->x_mg = sample.x_mg;
    out->y_mg = sample.y_mg;
    out->z_mg = sample.z_mg;
    return ESP_OK;
}
