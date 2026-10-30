#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <vector>

extern "C" {
#include "jpeg_metadata.h"
}

namespace
{
struct Source {
    std::vector<uint8_t> bytes;
    size_t calls = 0, fail_at = SIZE_MAX;
};
bool read_at(void *ctx, size_t offset, uint8_t *data, size_t n)
{
    auto &s = *static_cast<Source *>(ctx);
    if (s.calls++ == s.fail_at || offset > s.bytes.size() || n > s.bytes.size() - offset)
        return false;
    memcpy(data, s.bytes.data() + offset, n);
    return true;
}
std::vector<uint8_t> app1(unsigned orientation, bool little = true)
{
    std::vector<uint8_t> out = {0xff,
                                0xe1,
                                0,
                                34,
                                'E',
                                'x',
                                'i',
                                'f',
                                0,
                                0,
                                'I',
                                'I',
                                42,
                                0,
                                8,
                                0,
                                0,
                                0,
                                1,
                                0,
                                0x12,
                                1,
                                3,
                                0,
                                1,
                                0,
                                0,
                                0,
                                (uint8_t) orientation,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0};
    if (!little) {
        out[10] = 'M';
        out[11] = 'M';
        out[12] = 0;
        out[13] = 42;
        out[14] = 0;
        out[17] = 8;
        out[18] = 0;
        out[19] = 1;
        out[20] = 1;
        out[21] = 0x12;
        out[22] = 0;
        out[23] = 3;
        out[24] = 0;
        out[27] = 1;
        out[28] = 0;
        out[29] = orientation;
    }
    return out;
}
Source jpeg(unsigned orientation = 0, bool little = true)
{
    Source s{{0xff, 0xd8}};
    if (orientation) {
        auto app = app1(orientation, little);
        s.bytes.insert(s.bytes.end(), app.begin(), app.end());
    }
    std::vector<uint8_t> frame = {0xff, 0xc0, 0, 17,   8, 0, 2,    0, 3,    3,    1,
                                  0x11, 0,    2, 0x11, 0, 3, 0x11, 0, 0xff, 0xda, 0,
                                  12,   3,    1, 0,    2, 0, 3,    0, 0,    63,   0};
    s.bytes.insert(s.bytes.end(), frame.begin(), frame.end());
    return s;
}
jpeg_metadata_error_t parse(Source &s, jpeg_metadata_t &out)
{
    const jpeg_metadata_io_t io = {&s, read_at, s.bytes.size()};
    return jpeg_metadata_read(&io, &out);
}
}  // namespace

TEST(JpegMetadata, AbsentOrientationIsIdentity)
{
    auto s = jpeg();
    jpeg_metadata_t out;
    ASSERT_EQ(parse(s, out), JPEG_METADATA_OK);
    EXPECT_EQ(out.width, 3u);
    EXPECT_EQ(out.height, 2u);
    EXPECT_EQ(out.orientation, 1u);
}
TEST(JpegMetadata, AllOrientationsAndBothTiffEndiannesses)
{
    for (bool little : {false, true})
        for (unsigned o = 1; o <= 8; ++o) {
            auto s = jpeg(o, little);
            jpeg_metadata_t out;
            ASSERT_EQ(parse(s, out), JPEG_METADATA_OK) << little << ":" << o;
            EXPECT_EQ(out.orientation, o);
        }
}
TEST(JpegMetadata, AllEightMappingsArePinnedAgainstAsymmetricRaster)
{
    const char *expected[] = {"abcdef", "cbafed", "fedcba", "defabc",
                              "adbecf", "daebfc", "fcebda", "cfbead"};
    for (unsigned o = 1; o <= 8; ++o) {
        std::string result;
        for (unsigned y = 0; y < (o >= 5 ? 3u : 2u); ++y)
            for (unsigned x = 0; x < (o >= 5 ? 2u : 3u); ++x) {
                unsigned sx, sy;
                ASSERT_TRUE(jpeg_orientation_map(3, 2, o, x, y, &sx, &sy));
                result += "abcdef"[sy * 3 + sx];
            }
        EXPECT_EQ(result, expected[o - 1]) << o;
    }
}
TEST(JpegMetadata, InvalidViewCoordinatesAndArguments)
{
    unsigned x, y;
    EXPECT_FALSE(jpeg_orientation_map(3, 2, 0, 0, 0, &x, &y));
    EXPECT_FALSE(jpeg_orientation_map(3, 2, 9, 0, 0, &x, &y));
    EXPECT_FALSE(jpeg_orientation_map(3, 2, 6, 2, 0, &x, &y));
    EXPECT_FALSE(jpeg_orientation_map(3, 2, 1, 0, 2, &x, &y));
    EXPECT_FALSE(jpeg_orientation_map(0, 2, 1, 0, 0, &x, &y));
    EXPECT_FALSE(jpeg_orientation_map(3, 2, 1, 0, 0, nullptr, &y));
}
TEST(JpegMetadata, BadExifTypeCountValueAndOffsetRejected)
{
    for (auto change :
         std::vector<std::pair<size_t, uint8_t>>{{24, 4}, {26, 2}, {30, 0}, {16, 255}}) {
        auto s = jpeg(6);
        jpeg_metadata_t out;
        s.bytes[change.first] = change.second;
        EXPECT_NE(parse(s, out), JPEG_METADATA_OK) << change.first;
    }
    auto s = jpeg(9);
    jpeg_metadata_t out;
    EXPECT_EQ(parse(s, out), JPEG_METADATA_INVALID);
}
TEST(JpegMetadata, DuplicateOrientationRefused)
{
    auto s = jpeg(6);
    auto app = app1(6);
    jpeg_metadata_t out;
    s.bytes.insert(s.bytes.begin() + 2, app.begin(), app.end());
    EXPECT_EQ(parse(s, out), JPEG_METADATA_INVALID);
}
TEST(JpegMetadata, EveryHeaderTruncationAndIoFailureIsRejected)
{
    auto original = jpeg(6);
    jpeg_metadata_t out;
    for (size_t n = 0; n < original.bytes.size(); ++n) {
        auto s = original;
        s.bytes.resize(n);
        EXPECT_NE(parse(s, out), JPEG_METADATA_OK) << n;
    }
    auto good = original;
    ASSERT_EQ(parse(good, out), JPEG_METADATA_OK);
    for (size_t i = 0; i < good.calls; ++i) {
        auto bad = original;
        bad.fail_at = i;
        EXPECT_EQ(parse(bad, out), JPEG_METADATA_READ_FAILED) << i;
    }
}
TEST(JpegMetadata, ProgressiveAndInvalidSofRejected)
{
    auto s = jpeg();
    jpeg_metadata_t out;
    s.bytes[3] = 0xc2;
    EXPECT_EQ(parse(s, out), JPEG_METADATA_UNSUPPORTED);
    s = jpeg();
    s.bytes[6] = 12;
    EXPECT_EQ(parse(s, out), JPEG_METADATA_UNSUPPORTED);
    s = jpeg();
    s.bytes[10] = 0;
    EXPECT_EQ(parse(s, out), JPEG_METADATA_LIMIT);
}
TEST(JpegMetadata, SourceMarkerAndPaddingLimitsAreBounded)
{
    auto s = jpeg();
    jpeg_metadata_t out;
    jpeg_metadata_io_t io{&s, read_at, 32u * 1024u * 1024u + 1};
    EXPECT_EQ(jpeg_metadata_read(&io, &out), JPEG_METADATA_LIMIT);
    EXPECT_EQ(s.calls, 0u);
    s.bytes = {0xff, 0xd8};
    for (int i = 0; i < 4096; ++i) {
        s.bytes.push_back(0xff);
        s.bytes.push_back(1);
    }
    EXPECT_EQ(parse(s, out), JPEG_METADATA_LIMIT);
    s.bytes.assign(4u * 1024u * 1024u + 8, 0xff);
    s.bytes[1] = 0xd8;
    EXPECT_EQ(parse(s, out), JPEG_METADATA_LIMIT);
}
TEST(JpegMetadata, ScaleBudgetAndOddFloorDimensions)
{
    jpeg_metadata_t info{6192, 4128, 6};
    unsigned scale;
    size_t n;
    ASSERT_TRUE(jpeg_metadata_scale(&info, 5u * 1024u * 1024u, &scale, &n));
    EXPECT_EQ(scale, 2u);
    EXPECT_EQ(n, 1548u * 1032u * 3u);
    ASSERT_TRUE(jpeg_metadata_scale(&info, 2u * 1024u * 1024u, &scale, &n));
    EXPECT_EQ(scale, 3u);
    EXPECT_FALSE(jpeg_metadata_scale(&info, 100, &scale, &n));
    info = {5935, 3957, 1};
    ASSERT_TRUE(jpeg_metadata_scale(&info, 5u * 1024u * 1024u, &scale, &n));
    EXPECT_EQ(scale, 2u);
    EXPECT_EQ(n, 1483u * 989u * 3u);
    info.orientation = 0;
    EXPECT_FALSE(jpeg_metadata_scale(&info, SIZE_MAX, &scale, &n));
    info = {16384, 16384, 1};
    EXPECT_FALSE(jpeg_metadata_scale(&info, SIZE_MAX, &scale, &n));
}
