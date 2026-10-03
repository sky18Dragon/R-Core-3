#pragma once

#include "driver/gpio.h"

#define BOARD_HAL_NAME "read_pico"
#define BOARD_HAL_TYPE BOARD_TYPE_READ_PICO
#define BOARD_HAL_DISPLAY_TYPE "gc16"
#define BOARD_HAL_DISPLAY_ROTATION_DEG 0
#define BOARD_HAL_WAKEUP_KEY GPIO_NUM_NC
#define BOARD_HAL_WAKEUP_KEY_NAME "PMU power key (not enabled)"
#define BOARD_HAL_ROTATE_KEY GPIO_NUM_NC
#define BOARD_HAL_CLEAR_KEY GPIO_NUM_NC
#define BOARD_HAL_I2C_SDA_PIN GPIO_NUM_39
#define BOARD_HAL_I2C_SCL_PIN GPIO_NUM_40
#define BOARD_HAL_SD_CLK_PIN GPIO_NUM_38
#define BOARD_HAL_SD_CMD_PIN GPIO_NUM_42
#define BOARD_HAL_SD_D0_PIN GPIO_NUM_44

// No SPI/EPD aliases: this board uses the parallel LCD interface.
