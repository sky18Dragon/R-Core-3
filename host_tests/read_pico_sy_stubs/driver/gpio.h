#pragma once
#include <stdint.h>

#include "esp_err.h"
typedef int gpio_num_t;
#define GPIO_NUM_NC (-1)
#define GPIO_IS_VALID_GPIO(n) ((n) >= 0 && (n) < 49)
enum {
    GPIO_MODE_OUTPUT,
    GPIO_MODE_INPUT,
    GPIO_PULLUP_DISABLE,
    GPIO_PULLDOWN_DISABLE,
    GPIO_INTR_DISABLE
};
typedef struct {
    uint64_t pin_bit_mask;
    int mode, pull_up_en, pull_down_en, intr_type;
} gpio_config_t;
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t gpio_set_level(gpio_num_t, uint32_t);
int gpio_get_level(gpio_num_t);
esp_err_t gpio_config(const gpio_config_t *);
#ifdef __cplusplus
}
#endif
