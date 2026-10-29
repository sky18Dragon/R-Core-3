#include "read_pico_touch.h"

#include <string.h>

#include "cst836u.h"
#include "esp_log.h"
#include "read_pico_board.h"

#define READ_PICO_TOUCH_INT_GPIO GPIO_NUM_43
#define READ_PICO_TOUCH_INIT_LOCK_MS 100

static const char *TAG = "read_pico_touch";
static cst836u_handle_t s_touch;
static bool s_failed;

esp_err_t read_pico_touch_start(void)
{
    if (s_touch)
        return ESP_OK;
    if (s_failed)
        return ESP_ERR_INVALID_STATE;
    esp_err_t err = read_pico_board_lock(READ_PICO_TOUCH_INIT_LOCK_MS);
    if (err != ESP_OK)
        return err;  // busy: the caller may try again later
    i2c_master_bus_handle_t bus = NULL;
    err = read_pico_board_i2c_bus(&bus);
    if (err == ESP_OK) {
        cst836u_config_t config = CST836U_CONFIG_DEFAULT();
        config.int_gpio = READ_PICO_TOUCH_INT_GPIO;
        err = cst836u_init(bus, &config, &s_touch);
    }
    read_pico_board_unlock();
    if (err != ESP_OK) {
        s_touch = NULL;
        s_failed = true;
        // A controller held in reset (TP_RST low) or asleep does not answer;
        // this adapter never pulses TP_RST to find out.
        ESP_LOGW(TAG, "Touch unavailable (%s); disabled until restart", esp_err_to_name(err));
        return err;
    }
    cst836u_info_t info = {0};
    if (read_pico_board_lock(READ_PICO_TOUCH_INIT_LOCK_MS) == ESP_OK) {
        (void) cst836u_get_info(s_touch, &info);
        read_pico_board_unlock();
    }
    ESP_LOGI(TAG, "Touch ready: fw=0x%02x id=0x%02x type=0x%04x", info.fw, info.id, info.type);
    return ESP_OK;
}

bool read_pico_touch_ready(void)
{
    return s_touch != NULL;
}

esp_err_t read_pico_touch_read(read_pico_touch_sample_t *out, bool tracking)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    if (s_touch == NULL)
        return ESP_ERR_INVALID_STATE;
    // INT only pulses per report; gating on it missed every touch on the device.
    // Poll unconditionally like the vendor UI loop.
    (void) tracking;
    if (read_pico_board_lock(0) != ESP_OK)
        return ESP_ERR_TIMEOUT;
    cst836u_touch_t touch;
    esp_err_t err = cst836u_read(s_touch, &touch);
    read_pico_board_unlock();
    if (err != ESP_OK)
        return err;
    out->touched = touch.touched;
    out->count = touch.count;
    out->x = touch.x;
    out->y = touch.y;
    return ESP_OK;
}
