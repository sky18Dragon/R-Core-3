#include "read_pico_white_first.h"

#include <string.h>

int read_pico_white_first_gc16(EpdiyHighlevelState *hl, uint8_t *saved, size_t bytes,
                               bool panel_white, int temperature, bool (*healthy)(void))
{
    if (!panel_white) {
        memcpy(saved, hl->front_fb, bytes);
        memset(hl->front_fb, 0xff, bytes); // same as epd_hl_set_all_white()
        enum EpdDrawError flash = epd_hl_update_screen_full(hl, MODE_GC16, temperature);
        memcpy(hl->front_fb, saved, bytes);
        if (flash != EPD_DRAW_SUCCESS)
            return flash;
        if (healthy != NULL && !healthy())
            return -1;
    }
    return epd_hl_update_screen_from_white(hl, MODE_GC16, temperature);
}
