#pragma once

#include <stdint.h>

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int max_freq_khz;
} sdmmc_host_t;

typedef struct {
    int width;
    gpio_num_t clk;
    gpio_num_t cmd;
    gpio_num_t d0;
    gpio_num_t d1;
    gpio_num_t d2;
    gpio_num_t d3;
    uint32_t flags;
} sdmmc_slot_config_t;

#define SDMMC_FREQ_HIGHSPEED 40000
#define SDMMC_SLOT_FLAG_INTERNAL_PULLUP 1
#define SDMMC_HOST_DEFAULT() ((sdmmc_host_t) {0})
#define SDMMC_SLOT_CONFIG_DEFAULT() \
    ((sdmmc_slot_config_t) {        \
        .width = 4,                 \
        .clk = GPIO_NUM_NC,         \
        .cmd = GPIO_NUM_NC,         \
        .d0 = GPIO_NUM_NC,          \
        .d1 = GPIO_NUM_NC,          \
        .d2 = GPIO_NUM_NC,          \
        .d3 = GPIO_NUM_NC,          \
    })

#ifdef __cplusplus
}
#endif
