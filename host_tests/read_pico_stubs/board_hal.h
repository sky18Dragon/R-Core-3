#pragma once
#include <stdbool.h>
#include <time.h>

#include "esp_err.h"

#define BOARD_HAL_I2C_SDA_PIN 39
#define BOARD_HAL_I2C_SCL_PIN 40
typedef enum { BOARD_HAL_LED_POWER, BOARD_HAL_LED_ACTIVITY } board_hal_led_t;

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t board_hal_init(void);
esp_err_t board_hal_prepare_for_sleep(void);
bool board_hal_is_usb_connected(void);
#ifdef __cplusplus
}
#endif
