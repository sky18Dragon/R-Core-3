#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <vector>

extern "C" {
#include "read_pico_pattern.h"
}

namespace
{
uint8_t level(const std::vector<uint8_t> &frame, size_t x, size_t y)
{
    uint8_t b = frame[y * 608 + x / 2];
    return (x & 1) ? b & 15 : b >> 4;
}
std::vector<uint8_t> render(read_pico_pattern_t pattern)
{
    std::vector<uint8_t> frame(READ_PICO_FRAME_BYTES, 0xa5);
    EXPECT_TRUE(read_pico_pattern_render(pattern, frame.data(), frame.size()));
    return frame;
}
}  // namespace

TEST(ReadPicoPattern, InvalidArgumentsLeaveDestinationUnchanged)
{
    std::vector<uint8_t> frame(READ_PICO_FRAME_BYTES, 0xa5);
    EXPECT_FALSE(read_pico_pattern_render(READ_PICO_PATTERN_WHITE, nullptr, frame.size()));
    for (auto p : {static_cast<read_pico_pattern_t>(-1), READ_PICO_PATTERN_COUNT,
                   static_cast<read_pico_pattern_t>(100)})
        EXPECT_FALSE(read_pico_pattern_render(p, frame.data(), frame.size()));
    for (auto size : {size_t{0}, size_t{1}, size_t{READ_PICO_FRAME_BYTES - 1}})
        EXPECT_FALSE(read_pico_pattern_render(READ_PICO_PATTERN_GRAY16, frame.data(), size));
    EXPECT_TRUE(std::all_of(frame.begin(), frame.end(), [](uint8_t b) { return b == 0xa5; }));
}

TEST(ReadPicoPattern, AllPatternsPreserveBufferGuardsAndAreDeterministic)
{
    for (unsigned i = 0; i < READ_PICO_PATTERN_COUNT; ++i) {
        auto p = static_cast<read_pico_pattern_t>(i);
        std::vector<uint8_t> guarded(READ_PICO_FRAME_BYTES + 32, 0xa5);
        ASSERT_TRUE(read_pico_pattern_render(p, guarded.data() + 8, READ_PICO_FRAME_BYTES + 24));
        EXPECT_TRUE(
            std::all_of(guarded.begin(), guarded.begin() + 8, [](uint8_t b) { return b == 0xa5; }));
        EXPECT_TRUE(
            std::all_of(guarded.end() - 24, guarded.end(), [](uint8_t b) { return b == 0xa5; }));
        auto expected = render(p);
        EXPECT_TRUE(std::equal(expected.begin(), expected.end(), guarded.begin() + 8));
    }
}

TEST(ReadPicoPattern, SolidWhiteBlackAndMidgrayHaveExactLevels)
{
    const std::array<uint8_t, 3> bytes = {0xff, 0x00, 0x88};
    for (unsigned i = 0; i < bytes.size(); ++i) {
        auto frame = render(static_cast<read_pico_pattern_t>(i));
        EXPECT_TRUE(
            std::all_of(frame.begin(), frame.end(), [&](uint8_t b) { return b == bytes[i]; }));
    }
}

TEST(ReadPicoPattern, Gray16HasSixteenEqualWidthMonotonicVerticalBands)
{
    auto frame = render(READ_PICO_PATTERN_GRAY16);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; ++x)
            ASSERT_EQ(level(frame, x, y), x / 76) << x << ',' << y;
}

TEST(ReadPicoPattern, ParityHasVerticalHorizontalAndCheckerBands)
{
    auto frame = render(READ_PICO_PATTERN_PARITY);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; ++x) {
            size_t parity = y < 228 ? (x & 1) : y < 456 ? (y & 1) : ((x ^ y) & 1);
            ASSERT_EQ(level(frame, x, y), parity ? 15 : 0) << x << ',' << y;
        }
    EXPECT_EQ(frame.front(), 0x0f);  // PhotoFrame even pixel is HIGH nibble.
    EXPECT_EQ(frame[228 * 608], 0x00);
    EXPECT_EQ(frame[229 * 608], 0xff);
}

TEST(ReadPicoPattern, CornersHaveFullBorderWhiteCenterAndDistinctDotCounts)
{
    auto frame = render(READ_PICO_PATTERN_CORNERS);
    for (size_t y = 0; y < 684; ++y) {
        EXPECT_EQ(level(frame, 0, y), 0);
        EXPECT_EQ(level(frame, 1215, y), 0);
    }
    for (size_t x = 0; x < 1216; ++x) {
        EXPECT_EQ(level(frame, x, 0), 0);
        EXPECT_EQ(level(frame, x, 683), 0);
    }
    EXPECT_EQ(level(frame, 608, 342), 15);
    for (unsigned c = 0; c < 4; ++c) {
        size_t x = c & 1 ? 1134 : 16, y = c & 2 ? 610 : 16;
        for (unsigned dot = 0; dot < 4; ++dot)
            EXPECT_EQ(level(frame, x + dot * 16, y + 50), dot <= c ? 0 : 15);
        // First glyph: T has a full top row; B has a white last column.
        EXPECT_EQ(level(frame, x + 24, y), c & 2 ? 15 : 0);
        // Second glyph: L top row's second column is white; R's is black.
        EXPECT_EQ(level(frame, x + 42, y), c & 1 ? 0 : 15);
    }
}

TEST(ReadPicoPattern, RealFrameConversionSwapsParityNibblesWithoutChangingSource)
{
    auto frame = render(READ_PICO_PATTERN_PARITY);
    auto original = frame;
    std::vector<uint8_t> native(READ_PICO_FRAME_BYTES);
    read_pico_frame_layout_t layout{1216, 684, 608};
    ASSERT_EQ(read_pico_frame_convert(frame.data(), frame.size(), layout, native.data(),
                                      native.size(), layout, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    for (size_t i = 0; i < frame.size(); ++i)
        ASSERT_EQ(native[i], static_cast<uint8_t>((frame[i] << 4) | (frame[i] >> 4)));
    EXPECT_EQ(native.front(), 0xf0);
    EXPECT_EQ(frame, original);
}

TEST(ReadPicoPattern, NamesAreStableAndInvalidIdsAreSafe)
{
    const char *names[] = {"white",   "black",          "midgray",       "gray16",     "parity",
                           "corners", "gray16-labeled", "gray16-vendor", "gray16-page"};
    for (unsigned i = 0; i < READ_PICO_PATTERN_COUNT; ++i)
        EXPECT_STREQ(read_pico_pattern_name(static_cast<read_pico_pattern_t>(i)), names[i]);
    EXPECT_STREQ(read_pico_pattern_name(READ_PICO_PATTERN_COUNT), "invalid");
    EXPECT_STREQ(read_pico_pattern_name(static_cast<read_pico_pattern_t>(-1)), "invalid");
}

TEST(ReadPicoPattern, LabeledGray16PreservesExactCentralMeasurementBands)
{
    auto frame = render(READ_PICO_PATTERN_GRAY16_LABELED);
    auto original = render(READ_PICO_PATTERN_GRAY16);
    for (size_t y = 72; y < 612; ++y)
        for (size_t x = 0; x < 1216; ++x)
            ASSERT_EQ(level(frame, x, y), level(original, x, y)) << x << ',' << y;
    for (size_t x = 0; x < 1216; ++x) {
        EXPECT_EQ(level(frame, x, 0), 15);
        EXPECT_EQ(level(frame, x, 683), 15);
    }
}

TEST(ReadPicoPattern, VendorGrayControlKeepsOriginalUnlabeledGeometryExactly)
{
    EXPECT_EQ(render(READ_PICO_PATTERN_GRAY16_VENDOR), render(READ_PICO_PATTERN_GRAY16));
}

TEST(ReadPicoPattern, LabeledGray16HasDistinctRepeatedBinaryNumbersAndMarginGuides)
{
    auto frame = render(READ_PICO_PATTERN_GRAY16_LABELED);
    std::array<std::vector<uint8_t>, 16> labels;
    for (size_t code = 0; code < 16; ++code) {
        size_t x = code * 76;
        for (size_t y = 16; y < 44; ++y)
            for (size_t col = 16; col < 60; ++col) {
                auto v = level(frame, x + col, y);
                ASSERT_TRUE(v == 0 || v == 15);
                EXPECT_EQ(v, level(frame, x + col, y + 612));
                labels[code].push_back(v);
            }
        EXPECT_TRUE(std::find(labels[code].begin(), labels[code].end(), 0) != labels[code].end());
        for (size_t previous = 0; previous < code; ++previous)
            EXPECT_NE(labels[code], labels[previous]);
        EXPECT_EQ(level(frame, x, 64), 0);
        EXPECT_EQ(level(frame, x, 612), 0);
        EXPECT_EQ(level(frame, x + 2, 64), 15);
    }
    // 00 top row 01110, 01 top row second digit 00100, 15 first digit 00100.
    EXPECT_EQ(level(frame, 20, 16), 0);
    EXPECT_EQ(level(frame, 76 + 44, 16), 15);
    EXPECT_EQ(level(frame, 76 + 48, 16), 0);
    EXPECT_EQ(level(frame, 15 * 76 + 24, 16), 0);
}

TEST(ReadPicoPattern, LabeledGray16RealConversionPreservesEveryCodeAndSource)
{
    auto frame = render(READ_PICO_PATTERN_GRAY16_LABELED);
    auto original = frame;
    std::vector<uint8_t> native(READ_PICO_FRAME_BYTES);
    read_pico_frame_layout_t layout{1216, 684, 608};
    ASSERT_EQ(read_pico_frame_convert(frame.data(), frame.size(), layout, native.data(),
                                      native.size(), layout, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    for (size_t i = 0; i < frame.size(); ++i)
        ASSERT_EQ(native[i], static_cast<uint8_t>((frame[i] << 4) | (frame[i] >> 4)));
    EXPECT_EQ(frame, original);
}
