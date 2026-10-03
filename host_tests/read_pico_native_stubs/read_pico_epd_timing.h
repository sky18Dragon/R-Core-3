#pragma once
#include "epdiy.h"
typedef struct {
    int pclk_mhz;
} read_pico_epd_scan_t;
enum { READ_PICO_EPD_SCAN_FULL = 0 };
#ifdef __cplusplus
extern "C" {
#endif
void read_pico_epd_scan(int, int, read_pico_epd_scan_t *);
void read_pico_epd_init_lcd(const lcd_bus_config_t *, int, int, int, const read_pico_epd_scan_t *);
void read_pico_epd_set_pclk(int);
#ifdef __cplusplus
}
#endif
