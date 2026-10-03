#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void esp_rom_gpio_connect_out_signal(int, int, bool, bool);
#ifdef __cplusplus
}
#endif
