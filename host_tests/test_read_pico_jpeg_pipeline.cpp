#include <gtest/gtest.h>
#include <png.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>

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
extern const char *test_background_color;
extern esp_err_t test_processing_options_error;
extern unsigned test_processing_options_reads;
extern bool test_jpeg_media_present;
extern size_t test_jpeg_media_calls, test_jpeg_media_fail_at;
extern size_t test_heap_max_request, test_heap_request_limit;
extern bool test_jpeg_close_failed;
extern size_t test_image_close_calls, test_image_close_fail_at, test_image_close_media_loss_at;
extern float test_gray_levels[16];
extern bool test_gray_levels_present;
extern esp_err_t test_gray_levels_error;
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
        test_background_color = "white";
        test_processing_options_error = ESP_OK;
        test_processing_options_reads = 0;
        test_jpeg_media_present = true;
        test_jpeg_media_calls = 0;
        test_jpeg_media_fail_at = SIZE_MAX;
        test_heap_max_request = 0;
        test_heap_request_limit = SIZE_MAX;
        test_jpeg_close_failed = false;
        test_image_close_calls = 0;
        test_image_close_fail_at = SIZE_MAX;
        test_image_close_media_loss_at = SIZE_MAX;
        test_gray_levels_present = false;
        test_gray_levels_error = ESP_OK;
        fake_display_reset();
        ASSERT_EQ(image_processor_init(), ESP_OK);
    }
    void TearDown() override
    {
        test_processing_options_error = ESP_OK;
        test_gray_levels_present = false;
        test_gray_levels_error = ESP_OK;
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

TEST_F(ReadPicoJpegPipeline, PngOptionsReadErrorOrBusyCancelsWithoutShowingAndCanRetry)
{
    auto png = oriented_png(fixture(), 1);
    for (esp_err_t failure : {ESP_FAIL, ESP_ERR_INVALID_STATE, ESP_ERR_INVALID_ARG}) {
        fake_display_reset(); test_processing_options_reads = 0;
        test_processing_options_error = failure;
        EXPECT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
                  DITHER_FLOYD_STEINBERG, nullptr), failure);
        EXPECT_EQ(test_processing_options_reads, 1u);
        EXPECT_EQ(fake_display_begin_count(), 0);
        EXPECT_FALSE(fake_display_was_shown());
        EXPECT_STREQ(image_processor_get_last_error(), "Processing options unavailable or invalid");
        test_processing_options_error = ESP_OK;
        EXPECT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
                  DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
        EXPECT_TRUE(fake_display_was_shown());
    }
}

TEST_F(ReadPicoJpegPipeline, PngOptionsFailureDoesNotPublishOutputOrModifySource)
{
    auto png = oriented_png(fixture(), 1); write(png);
    test_processing_options_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(image_processor_process(path.c_str(), output.c_str(), DITHER_FLOYD_STEINBERG),
              ESP_ERR_INVALID_STATE);
    EXPECT_EQ(access(output.c_str(), F_OK), -1); EXPECT_EQ(load(path), png);
    EXPECT_FALSE(fake_display_was_shown()); EXPECT_EQ(test_processing_options_reads, 1u);
}

TEST_F(ReadPicoJpegPipeline, FixedExifJpegCoverDoesNotReadProductLayoutOptions)
{
    write(fixture(6)); test_processing_options_error = ESP_FAIL;
    EXPECT_EQ(display(), ESP_OK); EXPECT_TRUE(fake_display_was_shown());
    EXPECT_EQ(test_processing_options_reads, 0u);
}

TEST_F(ReadPicoJpegPipeline, PngFitUsesOneOwnedLayoutAndBackgroundRead)
{
    auto png = oriented_png(fixture(), 1); test_scale_mode = SCALE_MODE_FIT;
    for (const char *background : {"white", "black"}) {
        test_background_color = background; test_processing_options_reads = 0;
        ASSERT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
                  DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
        EXPECT_EQ(test_processing_options_reads, 1u);
        auto pixels = frame(); ASSERT_FALSE(pixels.empty());
        EXPECT_EQ(pixels[0], strcmp(background, "black") == 0 ? 0 : 255);
    }
}

TEST_F(ReadPicoJpegPipeline, PngOptionsFailurePreservesPreviouslyCapturedDisplay)
{
    auto png = oriented_png(fixture(), 1);
    ASSERT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
              DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    auto previous = frame(); int begins = fake_display_begin_count();
    test_processing_options_error = ESP_FAIL;
    EXPECT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
              DITHER_FLOYD_STEINBERG, nullptr), ESP_FAIL);
    EXPECT_EQ(fake_display_begin_count(), begins); EXPECT_EQ(frame(), previous);
}

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

namespace {
struct OwnedAsset {
    std::string directory, image, preview, typed, sidecar;
    OwnedAsset() {
        char path[] = "/tmp/read-pico-asset-XXXXXX";
        const char *created = mkdtemp(path);
        EXPECT_NE(created, nullptr);
        directory = created ? created : "";
        image = directory + "/.rc3-upload-12345678-00000001.part";
        preview = directory + "/.rc3-upload-12345678-00000002.part";
        typed = directory + "/.rc3-upload-12345678-00000001.jpg";
        sidecar = directory + "/.rc3-upload-12345678-00000001.preview.jpg";
    }
    ~OwnedAsset() { if (!directory.empty()) std::filesystem::remove_all(directory); }
    static bool ready(void *) { return test_jpeg_media_present && !test_jpeg_close_failed; }
    static int create(void *, const char *p) { return open(p, O_CREAT | O_EXCL | O_WRONLY, 0600); }
    static int put(void *, int fd, const uint8_t *p, size_t n) { return (int)::write(fd, p, n); }
    static int sync(void *, int fd) { return fsync(fd); }
    static int close(void *, int fd) { return ::close(fd); }
    static void uncertain(void *) { test_jpeg_close_failed = true; }
    static int publish(void *, const char *from, const char *to) {
        if (link(from, to) != 0) return -1; // Host emulates FAT no-replace rename.
        return unlink(from);
    }
    upload_file_t complete(const std::string &path, const std::vector<uint8_t> &data) {
        upload_file_io_t io = {nullptr, ready, create, put, sync, close, uncertain, publish};
        upload_file_t owner{};
        EXPECT_TRUE(upload_file_begin(&owner, &io, path.c_str(), 5u * 1024u * 1024u));
        EXPECT_TRUE(upload_file_write(&owner, data.data(), data.size()));
        EXPECT_TRUE(upload_file_finish(&owner, data.size()));
        return owner;
    }
};
}
TEST_F(ReadPicoJpegPipeline, OwnedUploadPublishesThenDecodesAndCurrentServesDedicatedPreview) {
    OwnedAsset files; auto bytes = fixture();
    auto image = files.complete(files.image, bytes), preview = files.complete(files.preview, bytes);
    ASSERT_EQ(display_flow_present_asset(&image, &preview, IMAGE_FORMAT_JPG,
                                         DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_TRUE(fake_display_was_shown()); EXPECT_TRUE(image.published); EXPECT_TRUE(preview.published);
    EXPECT_EQ(load(files.typed), bytes); EXPECT_EQ(load(files.sidecar), bytes);
    EXPECT_STREQ(fake_display_pub_display_name(), files.typed.c_str());
    const char *content_type = nullptr;
    FILE *fp = display_flow_open_current(&content_type);
    ASSERT_NE(fp, nullptr); EXPECT_STREQ(content_type, "image/jpeg");
    uint8_t magic[2]; EXPECT_EQ(fread(magic, 1, 2, fp), 2u);
    EXPECT_EQ(magic[0], 0xff); EXPECT_EQ(magic[1], 0xd8); EXPECT_EQ(fclose(fp), 0);
}
TEST_F(ReadPicoJpegPipeline, OwnedMagicMismatchDoesNotPublishOrBeginDisplay) {
    OwnedAsset files; auto bytes = fixture(); auto image = files.complete(files.image, bytes);
    EXPECT_EQ(display_flow_present_asset(&image, nullptr, IMAGE_FORMAT_PNG,
                                         DITHER_FLOYD_STEINBERG), ESP_ERR_NOT_SUPPORTED);
    EXPECT_FALSE(image.published); EXPECT_EQ(load(files.image), bytes);
    EXPECT_FALSE(std::filesystem::exists(files.typed)); EXPECT_EQ(fake_display_begin_count(), 0);
}
TEST_F(ReadPicoJpegPipeline, OwnedMagicCloseFailureKeepsStageAndNeverBeginsDisplay) {
    OwnedAsset files; auto bytes = fixture(); auto image = files.complete(files.image, bytes);
    test_image_close_fail_at = 0;
    EXPECT_NE(display_flow_present_asset(&image, nullptr, IMAGE_FORMAT_JPG,
                                         DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_TRUE(test_jpeg_close_failed); EXPECT_FALSE(image.published);
    EXPECT_EQ(load(files.image), bytes); EXPECT_EQ(fake_display_begin_count(), 0);
}
TEST_F(ReadPicoJpegPipeline, OwnedDecodeFailureRetainsPublishedFileAndPreviousCurrent) {
    OwnedAsset files;
    auto bytes = fixture(); bytes.resize(40); auto image = files.complete(files.image, bytes);
    fake_display_set_current_image("previous.jpg");
    EXPECT_NE(display_flow_present_asset(&image, nullptr, IMAGE_FORMAT_JPG,
                                         DITHER_FLOYD_STEINBERG), ESP_OK);
    EXPECT_TRUE(image.published); EXPECT_EQ(load(files.typed), bytes);
    EXPECT_FALSE(fake_display_was_shown()); EXPECT_EQ(fake_display_begin_count(), 0);
    char current[512]; bool known;
    ASSERT_TRUE(display_manager_copy_current_image(current, sizeof(current), &known));
    EXPECT_TRUE(known); EXPECT_STREQ(current, "previous.jpg");
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

TEST_F(ReadPicoJpegPipeline, MemoryJpegMatchesFileForEveryExifAndDisplayOrientation)
{
    for (auto orientation : {DISPLAY_ORIENTATION_LANDSCAPE, DISPLAY_ORIENTATION_PORTRAIT})
        for (unsigned exif = 1; exif <= 8; ++exif) {
            SCOPED_TRACE(std::to_string(exif) + ":" + std::to_string(orientation));
            test_display_orientation = orientation;
            auto bytes = fixture(exif);
            const auto original = bytes;
            write(bytes);
            fake_display_reset();
            ASSERT_EQ(display(), ESP_OK);
            const auto expected = frame();
            fake_display_reset();
            test_jpeg_media_calls = test_image_close_calls = 0;
            ASSERT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                                        DITHER_FLOYD_STEINBERG, nullptr), ESP_OK)
                << image_processor_get_last_error();
            EXPECT_EQ(frame(), expected);
            EXPECT_TRUE(fake_display_was_shown());
            EXPECT_EQ(bytes, original);
            EXPECT_EQ(test_jpeg_media_calls, 0u);
            EXPECT_EQ(test_image_close_calls, 0u);
        }
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegDoesNotDependOnMountedOrHealthySd)
{
    const auto bytes = fixture(6);
    test_jpeg_media_present = false;
    test_jpeg_close_failed = true;
    ASSERT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                                DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    EXPECT_EQ(test_jpeg_media_calls, 0u);
    EXPECT_EQ(test_image_close_calls, 0u);
    EXPECT_TRUE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegForcesCoverAndPreservesPublishContract)
{
    const auto bytes = fixture(6);
    display_publish_t pub{};
    pub.display_name = "memory-photo";
    ASSERT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                                DITHER_FLOYD_STEINBERG, &pub), ESP_OK);
    const auto expected = frame();
    EXPECT_STREQ(fake_display_pub_display_name(), "memory-photo");
    fake_display_reset();
    test_scale_mode = SCALE_MODE_FIT;
    ASSERT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                                DITHER_FLOYD_STEINBERG, &pub), ESP_OK);
    EXPECT_EQ(frame(), expected);
    EXPECT_STREQ(fake_display_pub_display_name(), "memory-photo");
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegRejectsInvalidOrOverLimitInputBeforeAllocation)
{
    const auto bytes = fixture();
    EXPECT_EQ(image_processor_process_to_display(nullptr, bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(image_processor_process_to_display(bytes.data(), 0, IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_ERR_INVALID_ARG);
    // A deliberately overstated length must be rejected without touching bytes.
    for (size_t length : {size_t(32u * 1024u * 1024u + 1u), SIZE_MAX})
        EXPECT_NE(image_processor_process_to_display(bytes.data(), length, IMAGE_FORMAT_JPG,
                                                   DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    EXPECT_EQ(test_heap_max_request, 0u);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_FALSE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegRejectsUnsupportedOrInvalidMetadataBeforeAllocation)
{
    auto bytes = fixture(9);
    EXPECT_NE(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    bytes = fixture();
    bool patched = false;
    for (size_t i = 2; i + 1 < bytes.size(); ++i)
        if (bytes[i] == 0xff && bytes[i + 1] == 0xc0) {
            bytes[i + 1] = 0xc2;
            patched = true;
            break;
        }
    ASSERT_TRUE(patched);
    EXPECT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_ERR_NOT_SUPPORTED);
    bytes = fixture();
    patched = false;
    for (size_t i = 2; i + 8 < bytes.size(); ++i)
        if (bytes[i] == 0xff && bytes[i + 1] == 0xc0) {
            bytes[i + 5] = bytes[i + 7] = 0x40;
            bytes[i + 6] = bytes[i + 8] = 0; // 16384 squared exceeds 64M pixels.
            patched = true;
            break;
        }
    ASSERT_TRUE(patched);
    EXPECT_NE(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    EXPECT_EQ(test_heap_max_request, 0u);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_FALSE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegCorruptAndTruncatedDecodeNeverPublishes)
{
    std::vector<uint8_t> invalid{0xff, 0xd8, 0xff, 0xd9};
    EXPECT_NE(image_processor_process_to_display(invalid.data(), invalid.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    auto bytes = fixture();
    bytes.resize(bytes.size() / 2);
    const auto original = bytes;
    EXPECT_NE(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    EXPECT_EQ(bytes, original);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_FALSE(fake_display_was_shown());
    EXPECT_STRNE(image_processor_get_last_error(), "");
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegRgbAllocationFailureNeverBeginsDisplay)
{
    auto bytes = fixture();
    test_heap_request_limit = 1024;
    EXPECT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_ERR_NO_MEM);
    EXPECT_LE(test_heap_max_request, 5u * 1024u * 1024u);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_FALSE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegWorkAllocationFailureReleasesAllocatedRgb)
{
    auto bytes = fixture();
    bool patched = false;
    for (size_t i = 2; i + 8 < bytes.size(); ++i)
        if (bytes[i] == 0xff && bytes[i + 1] == 0xc0) {
            bytes[i + 5] = bytes[i + 7] = 0;
            bytes[i + 6] = bytes[i + 8] = 8;
            patched = true;
            break;
        }
    ASSERT_TRUE(patched);
    test_heap_request_limit = 1024; // 8x8x3 RGB succeeds, 3100-byte work fails.
    EXPECT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                               DITHER_FLOYD_STEINBERG, nullptr), ESP_ERR_NO_MEM);
    EXPECT_EQ(test_heap_max_request, 3100u);
    EXPECT_EQ(fake_display_begin_count(), 0);
    EXPECT_FALSE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, MemoryJpegDoesNotCopyLargeCompressedInput)
{
    auto bytes = fixture(6);
    bytes.resize(9u * 1024u * 1024u, 0x5a);
    const auto original = bytes;
    test_heap_request_limit = 5u * 1024u * 1024u;
    ASSERT_EQ(image_processor_process_to_display(bytes.data(), bytes.size(), IMAGE_FORMAT_JPG,
                                                DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    EXPECT_LE(test_heap_max_request, 5u * 1024u * 1024u);
    EXPECT_EQ(bytes, original);
    EXPECT_EQ(test_jpeg_media_calls, 0u);
    EXPECT_TRUE(fake_display_was_shown());
}

TEST_F(ReadPicoJpegPipeline, UnsupportedStreamFormatCannotDeletePersistentSource)
{
    const auto bytes = fixture();
    write(bytes);
    for (auto format : {IMAGE_FORMAT_UNKNOWN, IMAGE_FORMAT_BMP, IMAGE_FORMAT_EPD_GZ,
                        static_cast<image_format_t>(99)}) {
        EXPECT_EQ(display_flow_stream_file(path.c_str(), format, DITHER_FLOYD_STEINBERG,
                                          nullptr, true), ESP_ERR_NOT_SUPPORTED);
        EXPECT_EQ(load(path), bytes);
    }
    EXPECT_EQ(test_image_close_calls, 0u);
    EXPECT_EQ(test_jpeg_media_calls, 0u);
    EXPECT_EQ(test_heap_max_request, 0u);
    EXPECT_EQ(fake_display_begin_count(), 0);
}

TEST_F(ReadPicoJpegPipeline, InvalidStreamArgumentsCannotDeletePersistentSource)
{
    const auto bytes = fixture();
    write(bytes);
    EXPECT_EQ(display_flow_stream_file(nullptr, IMAGE_FORMAT_JPG, DITHER_FLOYD_STEINBERG,
                                      nullptr, true), ESP_ERR_INVALID_ARG);
    for (int algorithm : {-1, 4, 99})
        EXPECT_EQ(display_flow_stream_file(path.c_str(), IMAGE_FORMAT_JPG,
                                          static_cast<dither_algorithm_t>(algorithm), nullptr, true),
                  ESP_ERR_INVALID_ARG);
    EXPECT_EQ(load(path), bytes);
    EXPECT_EQ(test_image_close_calls, 0u);
    EXPECT_EQ(test_jpeg_media_calls, 0u);
    EXPECT_EQ(test_heap_max_request, 0u);
    EXPECT_EQ(fake_display_begin_count(), 0);
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

static double frame_mean()
{
    auto f = frame();
    double sum = 0;
    for (size_t i = 0; i < f.size(); i += 3) sum += f[i];
    return f.empty() ? -1 : sum / (f.size() / 3);
}

TEST_F(ReadPicoJpegPipeline, MeasuredGrayRampReplacesGammaModelOnDevice)
{
    auto png = oriented_png(fixture(), 1);
    ASSERT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
              DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    double gamma_mean = frame_mean();
    // Read Pico white-first ladder: mid codes are brighter than the 1.42 model,
    // so the same picture must be drawn with darker (lower) codes.
    const float ramp[16] = {0.028f, 0.034f, 0.042f, 0.065f, 0.095f, 0.132f, 0.201f, 0.253f,
                            0.337f, 0.394f, 0.470f, 0.522f, 0.581f, 0.610f, 0.643f, 0.645f};
    memcpy(test_gray_levels, ramp, sizeof(ramp));
    test_gray_levels_present = true;
    ASSERT_EQ(image_processor_reload_palette(), ESP_OK);
    fake_display_reset();
    ASSERT_EQ(image_processor_process_to_display(png.data(), png.size(), IMAGE_FORMAT_PNG,
              DITHER_FLOYD_STEINBERG, nullptr), ESP_OK);
    double lut_mean = frame_mean();
    EXPECT_LT(lut_mean, gamma_mean - 10.0) << gamma_mean << " -> " << lut_mean;
}

TEST_F(ReadPicoJpegPipeline, GrayRampReadErrorFailsPaletteReload)
{
    test_gray_levels_error = ESP_ERR_INVALID_RESPONSE;
    EXPECT_EQ(image_processor_reload_palette(), ESP_ERR_INVALID_RESPONSE);
}
