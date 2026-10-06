#include <gtest/gtest.h>
#include <png.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern "C" {
#include "config_manager.h"
#include "display_flow.h"
#include "fake_display_manager.h"
#include "image_processor.h"
#include "jpeg_metadata.h"
#include "jpeg_stream_probe.h"
#include "processing_settings.h"
extern int test_board_display_width, test_board_display_height;
extern const char *test_board_display_type;
extern display_orientation_t test_display_orientation;
extern scale_mode_t test_scale_mode;
extern bool test_jpeg_media_present;
extern size_t test_jpeg_media_calls, test_jpeg_media_fail_at;
extern size_t test_heap_max_request, test_heap_request_limit;
extern bool test_jpeg_close_failed;
extern size_t test_image_close_calls, test_image_close_fail_at, test_image_close_media_loss_at;
}

namespace
{
struct Source {
    std::vector<uint8_t> data;
    size_t offset = 0;
};
bool ready(void *)
{
    return true;
}
int read_bytes(void *ctx, uint8_t *out, size_t n)
{
    auto &s = *static_cast<Source *>(ctx);
    n = std::min(n, s.data.size() - s.offset);
    memcpy(out, s.data.data() + s.offset, n);
    s.offset += n;
    return n;
}
std::vector<uint8_t> load(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
std::vector<uint8_t> fixture(unsigned orientation = 1)
{
    auto bytes = load(std::string(JPEG_FIXTURE_DIR) + "/landscape.jpg");
    // Remove fixture APP1 only in this in-memory test copy, avoiding duplicate EXIF.
    size_t offset = 2;
    while (offset + 4 <= bytes.size()) {
        if (bytes[offset] != 0xff || bytes[offset + 1] == 0xda)
            break;
        size_t length = (size_t) bytes[offset + 2] * 256 + bytes[offset + 3];
        if (length < 2 || length + 2 > bytes.size() - offset)
            break;
        if (bytes[offset + 1] == 0xe1)
            bytes.erase(bytes.begin() + offset, bytes.begin() + offset + length + 2);
        else
            offset += length + 2;
    }
    std::vector<uint8_t> app = {0xff,
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
    bytes.insert(bytes.begin() + 2, app.begin(), app.end());
    return bytes;
}
std::vector<uint8_t> oriented_png(const std::vector<uint8_t> &bytes, unsigned o)
{
    Source s{bytes};
    jpeg_stream_probe_io_t io{&s, ready, read_bytes};
    union {
        max_align_t alignment;
        uint8_t data[3100];
    } work;
    // Fixture 1920x1080 requires 1/2 under the product's 5MiB RGB budget.
    std::vector<uint8_t> rgb(960u * 540u * 3u);
    auto r = jpeg_stream_decode_rgb(&io, work.data, 3100, 1, rgb.data(), rgb.size());
    EXPECT_EQ(r.error, JPEG_STREAM_OK);
    unsigned w = o >= 5 ? 540 : 960, h = o >= 5 ? 960 : 540;
    std::vector<uint8_t> oriented(w * h * 3u);
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x) {
            unsigned sx, sy;
            jpeg_orientation_map(960, 540, o, x, y, &sx, &sy);
            memcpy(oriented.data() + (y * w + x) * 3, rgb.data() + (sy * 960 + sx) * 3, 3);
        }
    png_image img{};
    img.version = PNG_IMAGE_VERSION;
    img.width = w;
    img.height = h;
    img.format = PNG_FORMAT_RGB;
    png_alloc_size_t size = 0;
    EXPECT_TRUE(png_image_write_to_memory(&img, nullptr, &size, 0, oriented.data(), 0, nullptr));
    std::vector<uint8_t> png(size);
    EXPECT_TRUE(png_image_write_to_memory(&img, png.data(), &size, 0, oriented.data(), 0, nullptr));
    png.resize(size);
    return png;
}
std::vector<uint8_t> frame()
{
    const uint8_t *p = fake_display_frame();
    if (!p)
        return {};
    return {p, p + (size_t) test_board_display_width * test_board_display_height * 3};
}
class ReadPicoJpegPipeline : public ::testing::Test
{
   protected:
    std::string path, output;
    void SetUp() override
    {
        char tmp[] = "/tmp/read-pico-jpeg-test-XXXXXX";
        int fd = mkstemp(tmp);
        ASSERT_GE(fd, 0);
        close(fd);
        path = tmp;
        output = path + ".png";
        test_board_display_width = 160;
        test_board_display_height = 96;
        test_board_display_type = "gc16";
        test_display_orientation = DISPLAY_ORIENTATION_LANDSCAPE;
        test_scale_mode = SCALE_MODE_COVER;
        test_jpeg_media_present = true;
        test_jpeg_media_calls = 0;
        test_jpeg_media_fail_at = SIZE_MAX;
        test_heap_max_request = 0;
        test_heap_request_limit = SIZE_MAX;
        test_jpeg_close_failed = false;
        test_image_close_calls = 0;
        test_image_close_fail_at = SIZE_MAX;
        test_image_close_media_loss_at = SIZE_MAX;
        fake_display_reset();
        ASSERT_EQ(image_processor_init(), ESP_OK);
    }
    void TearDown() override
    {
        remove(path.c_str());
        remove(output.c_str());
        fake_display_reset();
    }
    void write(const std::vector<uint8_t> &bytes)
    {
        FILE *f = fopen(path.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(fwrite(bytes.data(), 1, bytes.size(), f), bytes.size());
        ASSERT_EQ(fclose(f), 0);
    }
    esp_err_t display(bool release = false)
    {
        return display_flow_stream_file(path.c_str(), IMAGE_FORMAT_JPG, DITHER_FLOYD_STEINBERG,
                                        nullptr, release);
    }
};

TEST_F(ReadPicoJpegPipeline, FormatDetectionChecksMediaAndNeverConsumesMissingSource)
{
    write(fixture());
    EXPECT_EQ(image_processor_detect_format(path.c_str()), IMAGE_FORMAT_JPG);
    test_jpeg_media_present = false;
    EXPECT_EQ(image_processor_detect_format(path.c_str()), IMAGE_FORMAT_UNKNOWN);
    EXPECT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_FAIL);
    EXPECT_EQ(access(output.c_str(), F_OK), -1);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(image_processor_detect_format(nullptr), IMAGE_FORMAT_UNKNOWN);
}

TEST_F(ReadPicoJpegPipeline, BufferEntryRejectsInvalidAlgorithmBeforeDecodeOrLogIndex)
{
    auto bytes = fixture();
    for (int value : {-1, 4, 99}) {
        EXPECT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                         (dither_algorithm_t) value, nullptr), ESP_ERR_INVALID_ARG);
        EXPECT_EQ(fake_display_begin_count(), 0);
    }
}

TEST_F(ReadPicoJpegPipeline, ExistingJpegOutputIsNotTruncatedOrDeleted)
{
    write(fixture());
    const std::vector<uint8_t> sentinel = {4, 2, 7, 9};
    { std::ofstream f(output, std::ios::binary); f.write((const char *) sentinel.data(), sentinel.size()); }
    EXPECT_NE(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_EQ(load(output), sentinel);
    EXPECT_EQ(load(path), fixture());
}

TEST_F(ReadPicoJpegPipeline, ExistingPngOutputIsNotTruncatedOrDeletedOnFailure)
{
    write(oriented_png(fixture(), 1));
    auto before = load(path);
    const std::vector<uint8_t> sentinel = {5, 6, 1, 0};
    { std::ofstream f(output, std::ios::binary); f.write((const char *) sentinel.data(), sentinel.size()); }
    EXPECT_NE(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_EQ(load(output), sentinel);
    EXPECT_EQ(load(path), before);
}

TEST_F(ReadPicoJpegPipeline, PngReleaseSourceDoesNotDeletePersistentOriginal)
{
    write(oriented_png(fixture(), 1));
    auto before = load(path);
    ASSERT_EQ(image_processor_process_or_display_png(path.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_EQ(load(path), before);
}

TEST_F(ReadPicoJpegPipeline, JpegCloseFailurePreventsDisplayAndLocksMedia)
{
    write(fixture());
    auto before = load(path);
    test_image_close_fail_at = 0;
    EXPECT_NE(display(), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_TRUE(test_jpeg_close_failed);
    EXPECT_EQ(test_image_close_calls, 1u);
    EXPECT_EQ(load(path), before);
}

TEST_F(ReadPicoJpegPipeline, UnprocessedPngCloseFailureNeverCreatesOutputOrShows)
{
    write(oriented_png(fixture(), 1));
    auto before = load(path);
    test_image_close_fail_at = 1; // signature detection closes first
    EXPECT_NE(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_TRUE(test_jpeg_close_failed);
    EXPECT_EQ(access(output.c_str(), F_OK), -1);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_EQ(load(path), before);
}

TEST_F(ReadPicoJpegPipeline, PngFallbackCloseFailureAbortsWithoutDisplay)
{
    write(oriented_png(fixture(), 1));
    auto before = load(path);
    test_image_close_fail_at = 1; // fast-path closes, then full-source read closes
    EXPECT_NE(image_processor_process_or_display_png(path.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_TRUE(test_jpeg_close_failed);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_EQ(load(path), before);
}

TEST_F(ReadPicoJpegPipeline, ProcessedPngClosesBeforePublishAndFailsClosed)
{
    write(fixture());
    ASSERT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    auto before = load(output);
    test_image_close_calls = 0;
    test_image_close_fail_at = 0;
    fake_display_reset();
    EXPECT_NE(image_processor_process_or_display_png(output.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_EQ(test_image_close_calls, 1u);
    EXPECT_TRUE(test_jpeg_close_failed);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_EQ(load(output), before);
}

TEST_F(ReadPicoJpegPipeline, ProcessedPngMediaLossAtClosePreventsPublish)
{
    write(fixture());
    ASSERT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    test_image_close_calls = 0;
    test_image_close_media_loss_at = 0;
    fake_display_reset();
    EXPECT_NE(image_processor_process_or_display_png(output.c_str(), DITHER_FLOYD_STEINBERG, nullptr, false), ESP_OK);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_FALSE(test_jpeg_close_failed); // clean close, different from uncertain handle
}

TEST_F(ReadPicoJpegPipeline, MissingPngEndNeverPublishesFastOrFallbackPath)
{
    write(fixture());
    ASSERT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    auto processed = load(output);
    ASSERT_GT(processed.size(), 12u);
    processed.resize(processed.size() - 12); // remove IEND, not image rows
    write(processed);
    fake_display_reset();
    EXPECT_NE(image_processor_process_or_display_png(path.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_EQ(load(path), processed);
}

TEST_F(ReadPicoJpegPipeline, ProcessedPngWithTrailingBytesReadsClosesAndPreservesWholeSource)
{
    write(fixture());
    ASSERT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    auto processed = load(output);
    processed.resize(processed.size() + 8193, 0x5a);
    write(processed);
    test_image_close_calls = 0;
    fake_display_reset();
    ASSERT_EQ(image_processor_process_or_display_png(path.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_EQ(test_image_close_calls, 1u);
    EXPECT_TRUE(fake_display_was_shown());
    EXPECT_EQ(load(path), processed);
}
}  // namespace

TEST_F(ReadPicoJpegPipeline, EveryExifAndDisplayOrientationMatchesOrientedPngCover)
{
    for (auto display_orientation : {DISPLAY_ORIENTATION_LANDSCAPE, DISPLAY_ORIENTATION_PORTRAIT})
        for (unsigned o = 1; o <= 8; ++o) {
            test_display_orientation = display_orientation;
            auto jpeg = fixture(o), png = oriented_png(jpeg, o);
            write(jpeg);
            fake_display_reset();
            ASSERT_EQ(display(), ESP_OK) << o << ":" << image_processor_get_last_error();
            auto actual = frame();
            ASSERT_TRUE(fake_display_was_shown());
            fake_display_reset();
            ASSERT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
                                                         DITHER_FLOYD_STEINBERG, nullptr),
                      ESP_OK);
            EXPECT_EQ(actual, frame()) << o << ":" << display_orientation;
            EXPECT_EQ(load(path), jpeg);
        }
}
TEST_F(ReadPicoJpegPipeline, LargeCompressedSourceNotMaterializedOrDeleted)
{
    auto jpeg = fixture(6);
    jpeg.resize(9u * 1024u * 1024u, 0);
    write(jpeg);
    test_heap_request_limit = 5u * 1024u * 1024u;
    ASSERT_EQ(display(true), ESP_OK) << image_processor_get_last_error();
    EXPECT_LE(test_heap_max_request, 5u * 1024u * 1024u);
    // Regression guard for live I2C-backed status checks: previously >=2 per
    // 256-byte decoder/tail read. Cache hits must not perform physical polls.
    EXPECT_LT(test_jpeg_media_calls, jpeg.size() / 512);
    EXPECT_EQ(load(path), jpeg);
    EXPECT_TRUE(fake_display_was_shown());
    auto f = frame();
    ASSERT_FALSE(f.empty());
    for (size_t i = 0; i < f.size(); i += 3) {
        ASSERT_EQ(f[i], f[i + 1]);
        ASSERT_EQ(f[i], f[i + 2]);
        ASSERT_EQ(f[i] % 17, 0);
    }
}

TEST_F(ReadPicoJpegPipeline, ReadPicoJpegContractForcesCoverEvenWithLegacyFitSetting)
{
    auto jpeg = fixture(6);
    write(jpeg);
    ASSERT_EQ(display(), ESP_OK);
    auto expected = frame();
    fake_display_reset();
    test_scale_mode = SCALE_MODE_FIT;
    ASSERT_EQ(display(), ESP_OK);
    EXPECT_EQ(frame(), expected);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, MissingCorruptAndTruncatedSourcesNeverBeginDisplay)
{
    remove(path.c_str());
    EXPECT_NE(display(), ESP_OK);
    write({0xff, 0xd8, 0xff, 0xd9});
    EXPECT_NE(display(), ESP_OK);
    auto jpeg = fixture();
    jpeg.resize(jpeg.size() / 2);
    write(jpeg);
    EXPECT_NE(display(), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, InvalidExifAndProgressiveSourcesRefusedBeforeDisplay)
{
    auto jpeg = fixture(9);
    write(jpeg);
    EXPECT_NE(display(), ESP_OK);
    jpeg = fixture();
    for (size_t i = 2; i + 1 < jpeg.size(); ++i)
        if (jpeg[i] == 0xff && jpeg[i + 1] == 0xc0) {
            jpeg[i + 1] = 0xc2;
            break;
        }
    write(jpeg);
    EXPECT_NE(display(), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, AllocationFailurePreservesSourceAndNoOutput)
{
    auto jpeg = fixture();
    write(jpeg);
    test_heap_request_limit = 1024;
    EXPECT_EQ(display(), ESP_ERR_NO_MEM);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_NO_MEM);
    EXPECT_EQ(access(output.c_str(), F_OK), -1);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, MissingAndMidReadMediaNeverBeginDisplay)
{
    auto jpeg = fixture();
    write(jpeg);
    test_jpeg_media_present = false;
    EXPECT_NE(display(), ESP_OK);
    test_jpeg_media_present = true;
    test_jpeg_media_calls = 0;
    test_jpeg_media_fail_at = 30;
    EXPECT_NE(display(), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, SourceOutputSameInodeOrSymlinkAliasNeverTruncates)
{
    auto jpeg = fixture();
    write(jpeg);
    EXPECT_EQ(image_processor_process(path.c_str(), path.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_INVALID_ARG);
    ASSERT_EQ(link(path.c_str(), output.c_str()), 0);
    EXPECT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_INVALID_ARG);
    remove(output.c_str());
    ASSERT_EQ(symlink(path.c_str(), output.c_str()), 0);
    EXPECT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_INVALID_ARG);
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, FileToPngUsesExifCoverAndPreservesSource)
{
    auto jpeg = fixture(6);
    write(jpeg);
    ASSERT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_OK);
    ASSERT_EQ(image_processor_process_or_display_png(output.c_str(), DITHER_FLOYD_STEINBERG,
                                                     nullptr, false),
              ESP_OK);
    auto png_frame = frame();
    fake_display_reset();
    ASSERT_EQ(display(), ESP_OK);
    EXPECT_EQ(png_frame, frame());
    EXPECT_EQ(load(path), jpeg);
}
TEST_F(ReadPicoJpegPipeline, OversizedSparseSourceAndInvalidArgumentsRefused)
{
    write(fixture());
    FILE *f = fopen(path.c_str(), "r+b");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(ftruncate(fileno(f), 32u * 1024u * 1024u + 1), 0);
    fclose(f);
    EXPECT_NE(display(), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_EQ(
        image_processor_process_jpeg_file_to_display(nullptr, DITHER_FLOYD_STEINBERG, nullptr),
        ESP_ERR_INVALID_ARG);
    EXPECT_EQ(
        image_processor_process_jpeg_file_to_display(path.c_str(), (dither_algorithm_t) 9, nullptr),
        ESP_ERR_INVALID_ARG);
    EXPECT_EQ(image_processor_process(nullptr, output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_INVALID_ARG);
}

TEST_F(ReadPicoJpegPipeline, OversizedPngRefusedBeforeFastDisplayOrAllocation)
{
    write(oriented_png(fixture(), 1));
    FILE *f = fopen(path.c_str(), "r+b");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(ftruncate(fileno(f), 32u * 1024u * 1024u + 1), 0);
    ASSERT_EQ(fclose(f), 0);
    EXPECT_NE(image_processor_process_or_display_png(path.c_str(), DITHER_FLOYD_STEINBERG, nullptr, true), ESP_OK);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_NE(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_EQ(access(output.c_str(), F_OK), -1);
    EXPECT_EQ(test_heap_max_request, 0u);
}
