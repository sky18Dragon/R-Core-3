#include "read_pico_vendor_gray16.h"

#include <string.h>

#include "epdiy.h"
#include "read_pico_frame.h"

bool read_pico_vendor_gray16_page_render(uint8_t *frame, size_t size)
{
    if (frame == NULL || size < READ_PICO_FRAME_BYTES || epd_width() != 1216 ||
        epd_height() != 684 || epd_get_rotation() != EPD_ROT_LANDSCAPE)
        return false;
    memset(frame, 0xff, READ_PICO_FRAME_BYTES);
    epd_set_rotation(EPD_ROT_INVERTED_PORTRAIT);
    // Locked app_refresh.c geometry; omit font/menu dependencies explicitly.
    for (int i = 0; i < 16; ++i)
        epd_fill_rect((EpdRect) {40 + i * 37, 176, 37, 240}, (uint8_t) (i * 17), frame);
    epd_fill_rect((EpdRect) {40, 472, 292, 200}, 0, frame);
    epd_fill_rect((EpdRect) {352, 472, 292, 1}, 0, frame);
    epd_fill_rect((EpdRect) {352, 671, 292, 1}, 0, frame);
    epd_fill_rect((EpdRect) {352, 472, 1, 200}, 0, frame);
    epd_fill_rect((EpdRect) {643, 472, 1, 200}, 0, frame);
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 16; ++col)
            if ((row + col) % 2 == 0)
                epd_fill_rect((EpdRect) {40 + col * 40, 712 + row * 40, 40, 40}, 0, frame);
    for (int i = 0; i < 20; ++i)
        epd_fill_rect((EpdRect) {40, 902 + i * 6, 604, 1}, 0, frame);
    epd_set_rotation(EPD_ROT_LANDSCAPE);
    return true;
}

bool read_pico_vendor_gray16_render(uint8_t *frame, size_t size)
{
    if (frame == NULL || size < READ_PICO_FRAME_BYTES || epd_width() != READ_PICO_PANEL_WIDTH ||
        epd_height() != READ_PICO_PANEL_HEIGHT || epd_get_rotation() != EPD_ROT_LANDSCAPE)
        return false;
    memset(frame, 0xff, READ_PICO_FRAME_BYTES);
    // Same 8-bit color convention as MindReset/read_pico_firmware app_refresh.c
    // (locked 28cde682...), adapted to our original full-screen measurement geometry.
    // Vendor epd_draw_pixel takes the HIGH nibble of this 8-bit value, not the raw i.
    for (int i = 0; i < 16; ++i) {
        EpdRect rect = {.x = i * 76, .y = 0, .width = 76, .height = 684};
        epd_fill_rect(rect, (uint8_t) (i * 17), frame);
    }
    return true;
}
