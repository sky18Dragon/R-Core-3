#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <vector>

extern "C" {
#include "epd_board.h"
#include "epd_highlevel.h"
#include "epdiy.h"
#include "read_pico_pattern.h"
#include "read_pico_vendor_gray16.h"
}

namespace
{
EpdDisplay_t panel{1216, 684, 16, 24, nullptr};
const EpdBoardDefinition *selected_board;
epd_ctrl_state_t controls{};
int hardware_calls, draw_calls;
std::vector<uint8_t> captured_diff;
std::vector<bool> captured_rows;
std::vector<uint8_t> captured_columns;
int captured_mode, captured_temperature;
const EpdWaveform *captured_waveform;

class ReadPicoVendor : public testing::Test
{
    void SetUp() override
    {
        hardware_calls = draw_calls = 0;
        epd_init(nullptr, &panel, EPD_LUT_1K);  // Renderer/board binding fake; no IO.
        epd_set_rotation(EPD_ROT_LANDSCAPE);
    }
    void TearDown() override
    {
        EXPECT_EQ(hardware_calls, 0);
    }
};

struct Buffers {
    std::vector<uint8_t> front{std::vector<uint8_t>(READ_PICO_FRAME_BYTES)};
    std::vector<uint8_t> back{std::vector<uint8_t>(READ_PICO_FRAME_BYTES, 0xff)};
    std::vector<uint8_t> diff{std::vector<uint8_t>(2 * READ_PICO_FRAME_BYTES)};
    bool rows[684]{};
    uint8_t columns[608]{};
    EpdiyHighlevelState hl{front.data(), back.data(), diff.data(), rows, columns, nullptr};
};
}  // namespace

// Compile real, unchanged vendor epdiy.c/highlevel.c and real types. Only IO/renderer
// and assembly difference generation are fakes; this is not a LUT/DMA/power test.
extern "C" {
void epd_set_board(const EpdBoardDefinition *board)
{
    selected_board = board;
}
const EpdBoardDefinition *epd_current_board(void)
{
    return selected_board;
}
epd_ctrl_state_t *epd_ctrl_state(void)
{
    return &controls;
}
void epd_renderer_init(enum EpdInitOptions) {}
void epd_renderer_deinit(void) {}
void epd_clear_area(EpdRect)
{
    ++hardware_calls;
}
void epd_lcd_set_pixel_clock_MHz(int)
{
    ++hardware_calls;
}
void epd_leading_skip_discard(void) {}
int64_t esp_timer_get_time(void)
{
    return 0;
}

EpdRect epd_difference_image_cropped(const uint8_t *to, const uint8_t *from, EpdRect area,
                                     uint8_t *difference, bool *rows, uint8_t *columns)
{
    // Reference CPU substitute for diff.S, only to inspect HL reset/full flags.
    for (int y = 0; y < 684; ++y) {
        rows[y] = false;
        for (int x = 0; x < 1216; ++x) {
            size_t p = y * 608 + x / 2;
            uint8_t t = x & 1 ? to[p] >> 4 : to[p] & 15;
            uint8_t f = x & 1 ? from[p] >> 4 : from[p] & 15;
            difference[y * 1216 + x] = (f << 4) | t;
            rows[y] = rows[y] || t != f;
        }
    }
    memset(columns, 0, 608);
    return area;
}
void epd_difference_column_range(EpdRect area, int *start, int *stop)
{
    *start = area.x;
    *stop = area.x + area.width;
}
enum EpdDrawError epd_draw_base(EpdRect, const uint8_t *data, EpdRect, enum EpdDrawMode mode,
                                int temperature, const bool *rows, const uint8_t *columns,
                                const EpdWaveform *waveform)
{
    ++draw_calls;
    captured_diff.assign(data, data + 2 * READ_PICO_FRAME_BYTES);
    captured_rows.assign(rows, rows + 684);
    captured_columns.assign(columns, columns + 608);
    captured_mode = mode;
    captured_temperature = temperature;
    captured_waveform = waveform;
    return EPD_DRAW_SUCCESS;
}
}

TEST_F(ReadPicoVendor, ActualVendorBytesMatchPortableConversionForAllPixels)
{
    std::vector<uint8_t> source(READ_PICO_FRAME_BYTES), converted(READ_PICO_FRAME_BYTES);
    std::vector<uint8_t> vendor(READ_PICO_FRAME_BYTES + 32, 0xa5);
    ASSERT_TRUE(
        read_pico_pattern_render(READ_PICO_PATTERN_GRAY16_VENDOR, source.data(), source.size()));
    read_pico_frame_layout_t layout{1216, 684, 608};
    ASSERT_EQ(read_pico_frame_convert(source.data(), source.size(), layout, converted.data(),
                                      converted.size(), layout, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    ASSERT_TRUE(read_pico_vendor_gray16_render(vendor.data() + 16, READ_PICO_FRAME_BYTES));
    EXPECT_TRUE(std::equal(converted.begin(), converted.end(), vendor.begin() + 16));
    EXPECT_TRUE(
        std::all_of(vendor.begin(), vendor.begin() + 16, [](uint8_t b) { return b == 0xa5; }));
    EXPECT_TRUE(std::all_of(vendor.end() - 16, vendor.end(), [](uint8_t b) { return b == 0xa5; }));
    for (int y = 0; y < 684; ++y)
        for (int x = 0; x < 1216; x += 2)
            ASSERT_EQ(vendor[16 + y * 608 + x / 2], (x / 76) * 17);
}

TEST_F(ReadPicoVendor, ActualEightBitColorApiUsesHighNibbleForBothPixelParities)
{
    std::vector<uint8_t> fb(READ_PICO_FRAME_BYTES, 0xa5);
    for (int i = 0; i < 16; ++i) {
        epd_fill_rect(EpdRect{i * 2, 0, 2, 1}, static_cast<uint8_t>(i * 17), fb.data());
        EXPECT_EQ(fb[i], i * 17);
    }
    EXPECT_EQ(fb[16], 0xa5);
}

TEST_F(ReadPicoVendor, OfficialPageGeometryMatchesIndependentPackedReferenceAndRestoresRotation)
{
    std::vector<uint8_t> source(READ_PICO_FRAME_BYTES), converted(READ_PICO_FRAME_BYTES);
    std::vector<uint8_t> vendor(READ_PICO_FRAME_BYTES + 32, 0xa5);
    ASSERT_TRUE(
        read_pico_pattern_render(READ_PICO_PATTERN_GRAY16_PAGE, source.data(), source.size()));
    read_pico_frame_layout_t layout{1216, 684, 608};
    ASSERT_EQ(read_pico_frame_convert(source.data(), source.size(), layout, converted.data(),
                                      converted.size(), layout, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    ASSERT_TRUE(read_pico_vendor_gray16_page_render(vendor.data() + 16, READ_PICO_FRAME_BYTES));
    EXPECT_EQ(epd_get_rotation(), EPD_ROT_LANDSCAPE);
    EXPECT_TRUE(std::equal(converted.begin(), converted.end(), vendor.begin() + 16));
    EXPECT_TRUE(
        std::all_of(vendor.begin(), vendor.begin() + 16, [](uint8_t b) { return b == 0xa5; }));
    EXPECT_TRUE(std::all_of(vendor.end() - 16, vendor.end(), [](uint8_t b) { return b == 0xa5; }));
    auto gray = [&](int u, int v) {
        return epd_get_pixel(v, 683 - u, 1216, 684, vendor.data() + 16) >> 4;
    };
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(gray(40 + i * 37, 176), i);
        EXPECT_EQ(gray(76 + i * 37, 415), i);
    }
    EXPECT_EQ(gray(632, 200), 15);
    EXPECT_EQ(gray(39, 200), 15);
    EXPECT_EQ(gray(40, 472), 0);
    EXPECT_EQ(gray(353, 473), 15);
    EXPECT_EQ(gray(643, 671), 0);
    EXPECT_EQ(gray(40, 902), 0);
    EXPECT_EQ(gray(40, 903), 15);
}

TEST_F(ReadPicoVendor, OfficialPageLeftEdgeKeepsMarginAndFortyPixelCheckerPitch)
{
    std::vector<uint8_t> frame(READ_PICO_FRAME_BYTES);
    ASSERT_TRUE(read_pico_vendor_gray16_page_render(frame.data(), frame.size()));
    auto gray = [&](int u, int v) {
        return epd_get_pixel(v, 683 - u, 1216, 684, frame.data()) >> 4;
    };

    // Gray bars: a 40px white margin, then exact 37px codes. The source image
    // does not contain a wide dark plateau at the portrait left edge.
    EXPECT_EQ(gray(0, 200), 15);
    EXPECT_EQ(gray(39, 200), 15);
    EXPECT_EQ(gray(40, 200), 0);
    EXPECT_EQ(gray(76, 200), 0);
    EXPECT_EQ(gray(77, 200), 1);
    EXPECT_EQ(gray(113, 200), 1);
    EXPECT_EQ(gray(114, 200), 2);
    EXPECT_EQ(gray(631, 200), 15);
    EXPECT_EQ(gray(632, 200), 15);

    // Checker row 0 starts black after the margin; row 1 starts white. Both
    // alternate every 40px, rather than extending one color through u=120.
    EXPECT_EQ(gray(39, 732), 15);
    EXPECT_EQ(gray(40, 732), 0);
    EXPECT_EQ(gray(79, 732), 0);
    EXPECT_EQ(gray(80, 732), 15);
    EXPECT_EQ(gray(119, 732), 15);
    EXPECT_EQ(gray(120, 732), 0);
    EXPECT_EQ(gray(79, 772), 15);
    EXPECT_EQ(gray(80, 772), 0);
    EXPECT_EQ(gray(119, 772), 0);
    EXPECT_EQ(gray(120, 772), 15);

    // The solid block and hairlines also retain the same 40px left margin.
    EXPECT_EQ(gray(39, 500), 15);
    EXPECT_EQ(gray(40, 500), 0);
    EXPECT_EQ(gray(331, 500), 0);
    EXPECT_EQ(gray(332, 500), 15);
    EXPECT_EQ(gray(39, 902), 15);
    EXPECT_EQ(gray(40, 902), 0);
    EXPECT_EQ(gray(643, 902), 0);
    EXPECT_EQ(gray(644, 902), 15);
    EXPECT_EQ(gray(40, 903), 15);

    // The runtime edge probe relies on these six exact row-equivalence groups.
    // Prove the relationship across all 1216 pixels, not only the samples above.
    const int logical_pairs[][2] = {
        {0, 39}, {40, 76}, {77, 79}, {80, 113}, {114, 119}, {120, 127},
    };
    for (const auto &pair : logical_pairs) {
        auto first = frame.begin() + (683 - pair[0]) * 608;
        auto second = frame.begin() + (683 - pair[1]) * 608;
        EXPECT_TRUE(std::equal(first, first + 608, second));
    }
    for (size_t i = 0; i + 1 < sizeof(logical_pairs) / sizeof(logical_pairs[0]); ++i) {
        auto first = frame.begin() + (683 - logical_pairs[i][0]) * 608;
        auto next = frame.begin() + (683 - logical_pairs[i + 1][0]) * 608;
        EXPECT_FALSE(std::equal(first, first + 608, next));
    }
}

TEST_F(ReadPicoVendor, PageRejectsInvalidInputWithoutChangingRotationOrBuffer)
{
    std::vector<uint8_t> fb(READ_PICO_FRAME_BYTES, 0xa5);
    EXPECT_FALSE(read_pico_vendor_gray16_page_render(nullptr, fb.size()));
    EXPECT_FALSE(read_pico_vendor_gray16_page_render(fb.data(), fb.size() - 1));
    epd_set_rotation(EPD_ROT_PORTRAIT);
    EXPECT_FALSE(read_pico_vendor_gray16_page_render(fb.data(), fb.size()));
    EXPECT_EQ(epd_get_rotation(), EPD_ROT_PORTRAIT);
    EXPECT_TRUE(std::all_of(fb.begin(), fb.end(), [](uint8_t b) { return b == 0xa5; }));
}

TEST_F(ReadPicoVendor, BadBufferGeometryAndRotationAreRejectedWithoutMutationOrIo)
{
    std::vector<uint8_t> fb(READ_PICO_FRAME_BYTES, 0xa5);
    auto original = fb;
    EXPECT_FALSE(read_pico_vendor_gray16_render(nullptr, fb.size()));
    EXPECT_FALSE(read_pico_vendor_gray16_render(fb.data(), fb.size() - 1));
    for (auto rotation :
         {EPD_ROT_PORTRAIT, EPD_ROT_INVERTED_LANDSCAPE, EPD_ROT_INVERTED_PORTRAIT}) {
        epd_set_rotation(rotation);
        EXPECT_FALSE(read_pico_vendor_gray16_render(fb.data(), fb.size()));
    }
    EpdDisplay_t wrong{684, 1216, 16, 24, nullptr};
    epd_init(nullptr, &wrong, EPD_LUT_1K);
    epd_set_rotation(EPD_ROT_LANDSCAPE);
    EXPECT_FALSE(read_pico_vendor_gray16_render(fb.data(), fb.size()));
    EXPECT_EQ(fb, original);
    epd_init(nullptr, &panel, EPD_LUT_1K);
}

TEST_F(ReadPicoVendor, ActualFromWhiteAndFullProduceSameHlCallWithWhiteReference)
{
    Buffers a, b;
    ASSERT_TRUE(read_pico_vendor_gray16_render(a.front.data(), a.front.size()));
    b.front = a.front;  // Same allocated size; update HL pointers explicitly.
    b.hl.front_fb = b.front.data();
    ASSERT_EQ(epd_hl_update_screen_full(&a.hl, MODE_GC16, 28), EPD_DRAW_SUCCESS);
    auto diff = captured_diff, columns = captured_columns;
    auto rows = captured_rows;
    int mode = captured_mode;
    EXPECT_EQ(captured_temperature, 28);
    ASSERT_EQ(epd_hl_update_screen_from_white(&b.hl, MODE_GC16, 28), EPD_DRAW_SUCCESS);
    EXPECT_EQ(captured_diff, diff);
    EXPECT_EQ(captured_columns, columns);
    EXPECT_EQ(captured_rows, rows);
    EXPECT_TRUE(std::all_of(rows.begin(), rows.end(), [](bool dirty) { return dirty; }));
    EXPECT_TRUE(
        std::all_of(columns.begin(), columns.end(), [](uint8_t dirty) { return dirty == 0xff; }));
    EXPECT_EQ(captured_mode, mode);
    EXPECT_EQ(mode, MODE_PACKING_1PPB_DIFFERENCE | MODE_GC16);
    EXPECT_EQ(captured_temperature, 28);
    EXPECT_EQ(captured_waveform, nullptr);
    EXPECT_EQ(a.back, a.front);
    EXPECT_EQ(b.back, b.front);
    EXPECT_EQ(draw_calls, 2);
}

TEST_F(ReadPicoVendor, ActualFromWhiteOverwritesStaleBackBufferBeforeDifferenceGeneration)
{
    Buffers b;
    ASSERT_TRUE(read_pico_vendor_gray16_render(b.front.data(), b.front.size()));
    std::fill(b.back.begin(), b.back.end(), 0x00);
    ASSERT_EQ(epd_hl_update_screen_from_white(&b.hl, MODE_GC16, 28), EPD_DRAW_SUCCESS);
    EXPECT_TRUE(std::all_of(captured_diff.begin(), captured_diff.end(),
                            [](uint8_t b) { return (b >> 4) == 15; }));
    EXPECT_EQ(b.back, b.front);
    // This CPU behavior explains why physical untouched-white gating is mandatory.
}
