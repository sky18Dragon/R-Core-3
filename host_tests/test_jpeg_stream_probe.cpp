#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

extern "C" {
#include "jpeg_stream_probe.h"
#include "jpeg_buffered_source.h"
#include "jpeg_file_probe.h"
}

namespace {
struct Source {
    std::vector<uint8_t> bytes;
    size_t offset = 0, max_read = 256, calls = 0, fail_at = SIZE_MAX, media_at = SIZE_MAX;
    int mode = 0;
    int opens = 0, closes = 0, open_result = 3, close_result = 0;
    int64_t declared_size = -2;
};
bool ready(void *context)
{
    auto &s = *static_cast<Source *>(context);
    return s.calls < s.media_at;
}
int read_bytes(void *context, uint8_t *data, size_t size)
{
    auto &s = *static_cast<Source *>(context);
    if (s.calls++ == s.fail_at)
        return s.mode == 1 ? (int) size + 1 : s.mode == 2 ? 0 : -1;
    size = std::min({size, s.max_read, s.bytes.size() - s.offset});
    memcpy(data, s.bytes.data() + s.offset, size);
    s.offset += size;
    return (int) size;
}
Source fixture(const char *name = "landscape.jpg")
{
    std::ifstream file(std::string(JPEG_FIXTURE_DIR) + "/" + name, std::ios::binary);
    Source source;
    source.bytes.assign(std::istreambuf_iterator<char>(file), {});
    return source;
}
jpeg_stream_probe_result_t run(Source &source, unsigned scale = 2)
{
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    jpeg_stream_probe_io_t io = {&source, ready, read_bytes};
    return jpeg_stream_probe_run(&io, work.bytes, sizeof(work.bytes), scale);
}
int open_file(void *context)
{
    auto &s = *static_cast<Source *>(context);
    ++s.opens;
    return s.open_result;
}
int64_t file_size(void *context, int)
{
    auto &s = *static_cast<Source *>(context);
    return s.declared_size == -2 ? (int64_t) s.bytes.size() : s.declared_size;
}
int read_file(void *context, int, uint8_t *data, size_t size)
{
    return read_bytes(context, data, size);
}
int close_file(void *context, int)
{
    auto &s = *static_cast<Source *>(context);
    ++s.closes;
    return s.close_result;
}
jpeg_file_probe_result_t run_file(Source &source, jpeg_file_probe_session_t &session,
                                   bool allocate = true)
{
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    const jpeg_file_probe_io_t io = {&source, ready, open_file, file_size, read_file, close_file};
    return jpeg_file_probe_run(&session, &io, allocate ? work.bytes : nullptr, sizeof(work.bytes));
}
} // namespace

TEST(JpegStreamProbe, RealBaselineAllScalesAndCompletePixels)
{
    for (unsigned scale = 0; scale <= 3; ++scale) {
        auto source = fixture();
        ASSERT_FALSE(source.bytes.empty());
        auto original = source.bytes;
        auto r = run(source, scale);
        EXPECT_EQ(r.error, JPEG_STREAM_OK);
        EXPECT_EQ(r.width, 1920u);
        EXPECT_EQ(r.height, 1080u);
        EXPECT_EQ(r.pixels, (1920u >> scale) * (1080u >> scale));
        EXPECT_GT(r.blocks, 0u);
        EXPECT_LE(r.bytes_read, source.bytes.size());
        EXPECT_EQ(source.bytes, original);
    }
}

TEST(JpegStreamProbe, BufferedRealDecoderMatchesAllScaleFingerprints)
{
    for (unsigned scale = 0; scale <= 3; ++scale) {
        auto plain = fixture();
        auto expected = run(plain, scale);
        auto input = fixture();
        input.max_read = 4096;
        jpeg_stream_probe_io_t raw{&input, ready, read_bytes};
        uint8_t cache[4096];
        jpeg_buffered_source_t buffered{};
        ASSERT_TRUE(jpeg_buffered_source_init(&buffered, &raw, cache, sizeof(cache), input.bytes.size()));
        jpeg_stream_probe_io_t io{&buffered, jpeg_buffered_source_ready, jpeg_buffered_source_read};
        union { max_align_t alignment; uint8_t bytes[3100]; } work;
        auto actual = jpeg_stream_probe_run(&io, work.bytes, sizeof(work.bytes), scale);
        ASSERT_EQ(actual.error, JPEG_STREAM_OK);
        EXPECT_EQ(actual.rgb_crc32, expected.rgb_crc32);
        EXPECT_EQ(actual.pixels, expected.pixels);
        EXPECT_EQ(actual.bytes_read, expected.bytes_read);
        EXPECT_LT(buffered.read_calls, plain.calls);
    }
}
TEST(JpegStreamProbe, OddDimensionsUseDecoderFloorGeometry)
{
    auto source = fixture("portrait.jpg");
    auto r = run(source);
    ASSERT_EQ(r.error, JPEG_STREAM_OK);
    EXPECT_EQ(r.output_width, 201u);
    EXPECT_EQ(r.output_height, 302u);
    EXPECT_EQ(r.pixels, 201u * 302u);
}
TEST(JpegStreamProbe, ShortReadsProduceSameDecodedFingerprint)
{
    auto normal = fixture(), short_reads = fixture();
    short_reads.max_read = 7;
    auto expected = run(normal), actual = run(short_reads);
    ASSERT_EQ(actual.error, JPEG_STREAM_OK);
    EXPECT_EQ(actual.pixels, expected.pixels);
    EXPECT_EQ(actual.rgb_crc32, expected.rgb_crc32);
}
TEST(JpegStreamProbe, InvalidArgumentsNeverRead)
{
    auto source = fixture();
    uint8_t work[3100];
    jpeg_stream_probe_io_t io = {&source, ready, read_bytes};
    EXPECT_EQ(jpeg_stream_probe_run(nullptr, work, sizeof(work), 2).error, JPEG_STREAM_INVALID);
    EXPECT_EQ(jpeg_stream_probe_run(&io, nullptr, sizeof(work), 2).error, JPEG_STREAM_INVALID);
    EXPECT_EQ(jpeg_stream_probe_run(&io, work, 3099, 2).error, JPEG_STREAM_INVALID);
    EXPECT_EQ(jpeg_stream_probe_run(&io, work, sizeof(work), 4).error, JPEG_STREAM_INVALID);
    io.read = nullptr;
    EXPECT_EQ(jpeg_stream_probe_run(&io, work, sizeof(work), 2).error, JPEG_STREAM_INVALID);
    EXPECT_EQ(source.calls, 0u);
}
TEST(JpegStreamProbe, MissingMediaDoesNotRead)
{
    auto source = fixture();
    source.media_at = 0;
    EXPECT_EQ(run(source).error, JPEG_STREAM_MEDIA_FAILED);
    EXPECT_EQ(source.calls, 0u);
}

TEST(JpegStreamProbe, RgbRasterMatchesBlockFingerprintAndShortReads)
{
    auto normal = fixture(), short_source = fixture();
    short_source.max_read = 7;
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    std::vector<uint8_t> rgb(480u*270u*3u,0xa5), other(rgb.size(),0);
    jpeg_stream_probe_io_t io{&normal,ready,read_bytes};
    auto result=jpeg_stream_decode_rgb(&io,work.bytes,3100,2,rgb.data(),rgb.size());
    ASSERT_EQ(result.error,JPEG_STREAM_OK);
    io.context=&short_source;
    result=jpeg_stream_decode_rgb(&io,work.bytes,3100,2,other.data(),other.size());
    ASSERT_EQ(result.error,JPEG_STREAM_OK);
    EXPECT_EQ(rgb,other);
    EXPECT_NE(rgb,std::vector<uint8_t>(rgb.size(),0xa5));
    EXPECT_EQ(result.rgb_crc32,0u); // No probe CRC claimed for product decode.
}

TEST(JpegStreamProbe, InsufficientRgbCapacityNeverWritesPixels)
{
    auto source=fixture();
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    std::vector<uint8_t> rgb(100,0xa5);
    jpeg_stream_probe_io_t io{&source,ready,read_bytes};
    EXPECT_EQ(jpeg_stream_decode_rgb(&io,work.bytes,3100,2,rgb.data(),rgb.size()).error,
              JPEG_STREAM_DIMENSIONS_FAILED);
    EXPECT_EQ(rgb,std::vector<uint8_t>(100,0xa5));
    source.calls=0;
    EXPECT_EQ(jpeg_stream_decode_rgb(&io,work.bytes,3100,2,nullptr,100).error,JPEG_STREAM_INVALID);
    EXPECT_EQ(source.calls,0u);
}

TEST(JpegStreamProbe, OddRgbRasterCompletesAndHasBoundedWrites)
{
    auto source=fixture("portrait.jpg");
    union { max_align_t alignment; uint8_t bytes[3100]; } work;
    const size_t n=201u*302u*3u;
    std::vector<uint8_t> rgb(n+2,0xa5);
    jpeg_stream_probe_io_t io{&source,ready,read_bytes};
    auto r=jpeg_stream_decode_rgb(&io,work.bytes,3100,2,rgb.data()+1,n);
    EXPECT_EQ(r.error,JPEG_STREAM_OK); EXPECT_EQ(r.pixels,201u*302u);
    EXPECT_EQ(rgb.front(),0xa5); EXPECT_EQ(rgb.back(),0xa5);
}
TEST(JpegStreamProbe, ReadAndOversizedResultsAreLatched)
{
    for (size_t point : {0u, 2u, 20u, 200u}) {
        for (int mode : {0, 1}) {
            auto source = fixture();
            source.fail_at = point;
            source.mode = mode;
            EXPECT_EQ(run(source).error, JPEG_STREAM_READ_FAILED);
            EXPECT_EQ(source.calls, point + 1);
        }
    }
}
TEST(JpegStreamProbe, MediaLossStopsFurtherReads)
{
    for (size_t point : {1u, 5u, 100u, 500u}) {
        auto source = fixture();
        source.media_at = point;
        EXPECT_EQ(run(source).error, JPEG_STREAM_MEDIA_FAILED);
        EXPECT_EQ(source.calls, point);
    }
}
TEST(JpegStreamProbe, TruncatedAndInvalidDataReject)
{
    auto source = fixture();
    source.bytes.resize(source.bytes.size() / 2);
    EXPECT_NE(run(source).error, JPEG_STREAM_OK);
    source = fixture();
    source.bytes[0] = 0;
    EXPECT_EQ(run(source).error, JPEG_STREAM_PREPARE_FAILED);
}
TEST(JpegStreamProbe, ZeroReadDoesNotLoopOrSucceed)
{
    auto source = fixture();
    source.fail_at = 0;
    source.mode = 2;
    EXPECT_NE(run(source).error, JPEG_STREAM_OK);
    EXPECT_EQ(source.calls, 1u);
}

TEST(JpegFileProbe, SuccessClosesOnceAndKeepsOriginalBytes)
{
    auto source = fixture();
    auto original = source.bytes;
    jpeg_file_probe_session_t session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_OK);
    EXPECT_EQ(r.stream.error, JPEG_STREAM_OK);
    EXPECT_EQ(source.opens, 1);
    EXPECT_EQ(source.closes, 1);
    EXPECT_EQ(source.bytes, original);
    EXPECT_TRUE(session.attempted);
    EXPECT_FALSE(session.close_failed);
    EXPECT_EQ(run_file(source, session).error, JPEG_FILE_ALREADY_ATTEMPTED);
    EXPECT_EQ(source.opens, 1);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, AllocationAndNoMediaFailureConsumeAttemptWithoutOpen)
{
    for (bool allocate : {false, true}) {
        auto source = fixture();
        if (allocate)
            source.media_at = 0;
        jpeg_file_probe_session_t session = {};
        EXPECT_NE(run_file(source, session, allocate).error, JPEG_FILE_OK);
        EXPECT_TRUE(session.attempted);
        EXPECT_EQ(source.opens, 0);
        EXPECT_EQ(source.closes, 0);
        EXPECT_EQ(run_file(source, session).error, JPEG_FILE_ALREADY_ATTEMPTED);
    }
}
TEST(JpegFileProbe, MissingFileDoesNotCreateOrClose)
{
    auto source = fixture();
    source.open_result = -1;
    jpeg_file_probe_session_t session = {};
    EXPECT_EQ(run_file(source, session).error, JPEG_FILE_OPEN_FAILED);
    EXPECT_EQ(source.opens, 1);
    EXPECT_EQ(source.closes, 0);
    EXPECT_EQ(source.calls, 0u);
}
TEST(JpegFileProbe, InvalidAndTooLargeSizesCloseWithoutReading)
{
    for (int64_t size : {INT64_C(-1), INT64_C(0), INT64_C(33554433)}) {
        auto source = fixture();
        source.declared_size = size;
        jpeg_file_probe_session_t session = {};
        EXPECT_EQ(run_file(source, session).error, JPEG_FILE_SIZE_FAILED);
        EXPECT_EQ(source.closes, 1);
        EXPECT_EQ(source.calls, 0u);
    }
}
TEST(JpegFileProbe, ReadFailureClosesOnceAndRemainsPrimary)
{
    auto source = fixture();
    source.fail_at = 20;
    source.close_result = -1;
    jpeg_file_probe_session_t session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_STREAM_FAILED);
    EXPECT_EQ(r.stream.error, JPEG_STREAM_READ_FAILED);
    EXPECT_EQ(source.closes, 1);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(run_file(source, session).error, JPEG_FILE_ALREADY_ATTEMPTED);
}
TEST(JpegFileProbe, CloseFailureCannotBecomeSuccess)
{
    auto source = fixture();
    source.close_result = -1;
    jpeg_file_probe_session_t session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_CLOSE_FAILED);
    EXPECT_EQ(r.stream.error, JPEG_STREAM_OK);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, MediaFailureStillClosesOnce)
{
    auto source = fixture();
    source.media_at = 10;
    jpeg_file_probe_session_t session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_STREAM_FAILED);
    EXPECT_EQ(r.stream.error, JPEG_STREAM_MEDIA_FAILED);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, SizeBoundStopsReadBeforePhysicalEnd)
{
    auto source = fixture();
    source.declared_size = 100;
    jpeg_file_probe_session_t session = {};
    EXPECT_EQ(run_file(source, session).error, JPEG_FILE_STREAM_FAILED);
    EXPECT_LE(source.offset, 100u);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, PreexistingCloseLatchRejectsOpen)
{
    auto source = fixture();
    jpeg_file_probe_session_t session = {.attempted = false, .close_failed = true};
    EXPECT_EQ(run_file(source, session).error, JPEG_FILE_ALREADY_ATTEMPTED);
    EXPECT_EQ(source.opens, 0);
}

TEST(JpegFileProbe, TrailingBytesAreReadButNotReportedAsDecoded)
{
    auto source = fixture();
    source.bytes.resize(source.bytes.size() + 4096, 0x5a);
    jpeg_file_probe_session_t session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_OK);
    EXPECT_EQ(r.bytes_read, source.bytes.size());
    EXPECT_LT(r.stream.bytes_read, r.bytes_read);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, EOFErrorAfterAllBytesStillFails)
{
    auto reference = fixture();
    jpeg_file_probe_session_t session = {};
    ASSERT_EQ(run_file(reference, session).error, JPEG_FILE_OK);
    auto source = fixture();
    source.fail_at = reference.calls - 1;
    session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.bytes_read, source.bytes.size());
    EXPECT_EQ(r.error, JPEG_FILE_READ_FAILED);
    EXPECT_EQ(source.closes, 1);
}
TEST(JpegFileProbe, TrailingReadFailurePreservesCloseFailureSeparately)
{
    auto reference = fixture();
    reference.bytes.resize(reference.bytes.size() + 4096, 0x5a);
    jpeg_file_probe_session_t session = {};
    ASSERT_EQ(run_file(reference, session).error, JPEG_FILE_OK);
    auto source = fixture();
    source.bytes.resize(source.bytes.size() + 4096, 0x5a);
    source.fail_at = reference.calls - 5;
    source.close_result = -1;
    session = {};
    auto r = run_file(source, session);
    EXPECT_EQ(r.error, JPEG_FILE_READ_FAILED);
    EXPECT_EQ(r.stream.error, JPEG_STREAM_OK);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(source.closes, 1);
}
