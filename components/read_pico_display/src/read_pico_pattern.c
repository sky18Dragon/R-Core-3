#include "read_pico_pattern.h"

#include <string.h>

const char *read_pico_pattern_name(read_pico_pattern_t pattern)
{
    static const char *const names[] = {"white",          "black",         "midgray",
                                        "gray16",         "parity",        "corners",
                                        "gray16-labeled", "gray16-vendor", "gray16-page"};
    return (unsigned) pattern < READ_PICO_PATTERN_COUNT ? names[pattern] : "invalid";
}

static void pixel(uint8_t *frame, size_t x, size_t y, uint8_t level)
{
    size_t offset = y * (READ_PICO_PANEL_WIDTH / 2) + x / 2;
    uint8_t old = frame[offset];
    frame[offset] = x % 2 ? (old & 0xf0) | level : (old & 0x0f) | (level << 4);
}

static void rectangle(uint8_t *frame, size_t x, size_t y, size_t width, size_t height)
{
    for (size_t row = y; row < y + height; ++row)
        for (size_t col = x; col < x + width; ++col)
            pixel(frame, col, row, 0);
}

static void corner_label(uint8_t *frame, size_t x, size_t y, bool bottom, bool right)
{
    // Original minimal 5x7 glyphs (T, B, L, R), enlarged sixfold; no font dependency.
    static const uint8_t glyphs[][7] = {
        {31, 4, 4, 4, 4, 4, 4},
        {30, 17, 17, 30, 17, 17, 30},
        {16, 16, 16, 16, 16, 16, 31},
        {30, 17, 17, 30, 20, 18, 17},
    };
    const uint8_t *letters[] = {glyphs[bottom ? 1 : 0], glyphs[right ? 3 : 2]};
    for (size_t letter = 0; letter < 2; ++letter)
        for (size_t row = 0; row < 7; ++row)
            for (size_t col = 0; col < 5; ++col)
                if (letters[letter][row] & (1u << (4 - col)))
                    rectangle(frame, x + letter * 36 + col * 6, y + row * 6, 6, 6);
    // One/two/three/four dots identify TL/TR/BL/BR independently of text orientation.
    size_t dots = 1 + (bottom ? 2 : 0) + (right ? 1 : 0);
    for (size_t i = 0; i < dots; ++i)
        rectangle(frame, x + i * 16, y + 50, 8, 8);
}

static void gray_number(uint8_t *frame, size_t x, size_t y, unsigned code)
{
    // Original 5x7 decimal bitmap, fourfold scale; fixed two-digit labels 00..15.
    static const uint8_t digits[10][7] = {
        {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},  {14, 17, 1, 2, 4, 8, 31},
        {30, 1, 1, 14, 1, 1, 30},     {2, 6, 10, 18, 31, 2, 2}, {31, 16, 16, 30, 1, 1, 30},
        {14, 16, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},   {14, 17, 17, 14, 17, 17, 14},
        {14, 17, 17, 15, 1, 1, 14},
    };
    unsigned values[2] = {code / 10, code % 10};
    for (size_t i = 0; i < 2; ++i)
        for (size_t row = 0; row < 7; ++row)
            for (size_t col = 0; col < 5; ++col)
                if (digits[values[i]][row] & (1u << (4 - col)))
                    rectangle(frame, x + i * 24 + col * 4, y + row * 4, 4, 4);
}

bool read_pico_pattern_render(read_pico_pattern_t pattern, uint8_t *frame, size_t size)
{
    if ((unsigned) pattern >= READ_PICO_PATTERN_COUNT || frame == NULL ||
        size < READ_PICO_FRAME_BYTES)
        return false;
    if (pattern <= READ_PICO_PATTERN_MIDGRAY) {
        memset(frame,
               pattern == READ_PICO_PATTERN_WHITE   ? 0xff
               : pattern == READ_PICO_PATTERN_BLACK ? 0x00
                                                    : 0x88,
               READ_PICO_FRAME_BYTES);
        return true;
    }
    if (pattern == READ_PICO_PATTERN_GRAY16_PAGE) {
        // Official app_refresh geometry in inverted portrait. Text/menu omitted;
        // keep exact bar dimensions, black block, outline, checkerboard and thin lines.
        memset(frame, 0xff, READ_PICO_FRAME_BYTES);
        for (size_t v = 0; v < 1216; ++v)
            for (size_t u = 0; u < 684; ++u) {
                uint8_t level = 15;
                if (v >= 176 && v < 416 && u >= 40 && u < 632)
                    level = (u - 40) / 37;
                if (v >= 472 && v < 672 && u >= 40 && u < 332)
                    level = 0;
                if (u >= 352 && u < 644 && v >= 472 && v < 672 &&
                    (u == 352 || u == 643 || v == 472 || v == 671))
                    level = 0;
                // Official loop lets the last 40px checker extend past inner width.
                if (u >= 40 && u < 680 && v >= 712 && v < 872 &&
                    (((u - 40) / 40 + (v - 712) / 40) % 2 == 0))
                    level = 0;
                if (u >= 40 && u < 644 && v >= 902 && v <= 1016 && (v - 902) % 6 == 0)
                    level = 0;
                pixel(frame, v, 683 - u, level);
            }
        return true;
    }
    if (pattern == READ_PICO_PATTERN_CORNERS) {
        memset(frame, 0xff, READ_PICO_FRAME_BYTES);
        rectangle(frame, 0, 0, READ_PICO_PANEL_WIDTH, 2);
        rectangle(frame, 0, READ_PICO_PANEL_HEIGHT - 2, READ_PICO_PANEL_WIDTH, 2);
        rectangle(frame, 0, 0, 2, READ_PICO_PANEL_HEIGHT);
        rectangle(frame, READ_PICO_PANEL_WIDTH - 2, 0, 2, READ_PICO_PANEL_HEIGHT);
        corner_label(frame, 16, 16, false, false);
        corner_label(frame, READ_PICO_PANEL_WIDTH - 82, 16, false, true);
        corner_label(frame, 16, READ_PICO_PANEL_HEIGHT - 74, true, false);
        corner_label(frame, READ_PICO_PANEL_WIDTH - 82, READ_PICO_PANEL_HEIGHT - 74, true, true);
        return true;
    }
    memset(frame, 0xff, READ_PICO_FRAME_BYTES);
    for (size_t y = 0; y < READ_PICO_PANEL_HEIGHT; ++y) {
        for (size_t x = 0; x < READ_PICO_PANEL_WIDTH; ++x) {
            uint8_t level;
            if (pattern == READ_PICO_PATTERN_GRAY16_LABELED && (y < 72 || y >= 612))
                level = 15;
            else if (pattern == READ_PICO_PATTERN_GRAY16 ||
                     pattern == READ_PICO_PATTERN_GRAY16_LABELED ||
                     pattern == READ_PICO_PATTERN_GRAY16_VENDOR)
                level = (uint8_t) (x / (READ_PICO_PANEL_WIDTH / 16));
            else {
                // Top: vertical 1px; middle: horizontal 1px; bottom: 1px checkerboard.
                size_t parity = y < 228 ? x : y < 456 ? y : x + y;
                level = parity % 2 ? 15 : 0;
            }
            pixel(frame, x, y, level);
        }
    }
    if (pattern == READ_PICO_PATTERN_GRAY16_LABELED) {
        for (unsigned code = 0; code < 16; ++code) {
            size_t x = code * 76;
            gray_number(frame, x + 16, 16, code);
            gray_number(frame, x + 16, 628, code);
            // Guides live only in the white margins, never overwrite measurement bars.
            rectangle(frame, x, 64, 2, 8);
            rectangle(frame, x, 612, 2, 8);
        }
    }
    return true;
}
