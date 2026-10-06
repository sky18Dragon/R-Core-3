#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <vector>

extern "C" {
#include "jpeg_buffered_source.h"
}

namespace {
struct Source {
    std::vector<uint8_t> bytes;
    size_t offset = 0, calls = 0, max_request = 0, short_read = SIZE_MAX;
    bool present = true;
    int failure = 0;
};
bool ready(void *ctx) { return static_cast<Source *>(ctx)->present; }
int read_bytes(void *ctx, uint8_t *data, size_t size)
{
    auto &s = *static_cast<Source *>(ctx);
    ++s.calls;
    s.max_request = std::max(s.max_request, size);
    if (s.failure == 1) return -1;
    if (s.failure == 2) return 0;
    if (s.failure == 3) return (int) size + 1;
    size = std::min({size, s.short_read, s.bytes.size() - s.offset});
    memcpy(data, s.bytes.data() + s.offset, size);
    s.offset += size;
    if (s.failure == 4) s.present = false;
    return (int) size;
}
class JpegBufferedSource : public ::testing::Test {
protected:
    Source source;
    uint8_t buffer[4096];
    jpeg_buffered_source_t cached{};
    void SetUp() override {
        source.bytes.resize(4096 * 3 + 17);
        for (size_t i = 0; i < source.bytes.size(); ++i) source.bytes[i] = i % 251;
        jpeg_stream_probe_io_t io{&source, ready, read_bytes};
        ASSERT_TRUE(jpeg_buffered_source_init(&cached, &io, buffer, sizeof(buffer), source.bytes.size()));
    }
};
TEST_F(JpegBufferedSource, SmallDecoderReadsAreBatchedAndByteIdentical) {
    std::vector<uint8_t> actual(source.bytes.size());
    for (size_t i = 0; i < actual.size();) {
        size_t size = std::min<size_t>(256, actual.size() - i);
        ASSERT_EQ(jpeg_buffered_source_read(&cached, actual.data() + i, size), size);
        i += size;
    }
    EXPECT_EQ(actual, source.bytes);
    EXPECT_EQ(source.calls, 4u);
    EXPECT_EQ(cached.read_calls, 4u);
    EXPECT_EQ(cached.physical_bytes, actual.size());
    EXPECT_EQ(cached.delivered, actual.size());
    EXPECT_EQ(cached.remaining, 0u);
    EXPECT_EQ(source.max_request, 4096u);
    uint8_t tail;
    EXPECT_EQ(jpeg_buffered_source_read(&cached, &tail, 1), 0);
    EXPECT_EQ(source.calls, 4u); // caller owns the independent physical EOF check
}
TEST_F(JpegBufferedSource, ShortReadsAndCrossBufferRequestPreserveBytes) {
    source.short_read = 37;
    std::vector<uint8_t> actual(source.bytes.size() + 9);
    ASSERT_EQ(jpeg_buffered_source_read(&cached, actual.data(), actual.size()), source.bytes.size());
    actual.resize(source.bytes.size());
    EXPECT_EQ(actual, source.bytes);
    EXPECT_EQ(cached.physical_bytes, cached.delivered);
}
TEST_F(JpegBufferedSource, DeclaredSizeNeverReadsBeyondBoundary) {
    cached.remaining = 501;
    uint8_t out[700];
    ASSERT_EQ(jpeg_buffered_source_read(&cached, out, sizeof(out)), 501);
    EXPECT_EQ(source.offset, 501u);
    EXPECT_EQ(source.max_request, 501u);
}
TEST_F(JpegBufferedSource, NegativeEofAndOversizedReadsLatchFailure) {
    for (int mode : {1, 2, 3, 4}) {
        source.failure = mode;
        source.present = true;
        jpeg_stream_probe_io_t io{&source, ready, read_bytes};
        ASSERT_TRUE(jpeg_buffered_source_init(&cached, &io, buffer, sizeof(buffer), source.bytes.size()));
        uint8_t out[12];
        EXPECT_EQ(jpeg_buffered_source_read(&cached, out, sizeof(out)), -1);
        EXPECT_TRUE(cached.failed);
        source.failure = 0;
        source.present = true;
        EXPECT_FALSE(jpeg_buffered_source_ready(&cached));
        EXPECT_EQ(jpeg_buffered_source_read(&cached, out, 1), -1);
    }
}
TEST_F(JpegBufferedSource, RemovalDetectedAtNextRefillAndNeverRecoversAutomatically) {
    uint8_t out[4096];
    ASSERT_EQ(jpeg_buffered_source_read(&cached, out, 1), 1);
    source.present = false;
    // Read-ahead already obtained at most one block; no display is permitted here.
    ASSERT_EQ(jpeg_buffered_source_read(&cached, out, 4095), 4095);
    EXPECT_EQ(jpeg_buffered_source_read(&cached, out, 1), -1);
    EXPECT_EQ(source.calls, 1u);
    source.present = true;
    EXPECT_EQ(jpeg_buffered_source_read(&cached, out, 1), -1);
}
TEST_F(JpegBufferedSource, InvalidArgumentsAndZeroRequestDoNotRead) {
    uint8_t out;
    EXPECT_EQ(jpeg_buffered_source_read(&cached, &out, 0), 0);
    EXPECT_EQ(source.calls, 0u);
    EXPECT_EQ(jpeg_buffered_source_read(nullptr, &out, 1), -1);
    jpeg_buffered_source_t uninitialized{};
    EXPECT_EQ(jpeg_buffered_source_read(&uninitialized, &out, 1), -1);
    EXPECT_EQ(jpeg_buffered_source_read(&cached, nullptr, 1), -1);
    jpeg_stream_probe_io_t io{&source, ready, read_bytes};
    EXPECT_FALSE(jpeg_buffered_source_init(nullptr, &io, buffer, 4096, 1));
    EXPECT_FALSE(jpeg_buffered_source_init(&cached, nullptr, buffer, 4096, 1));
    EXPECT_FALSE(jpeg_buffered_source_init(&cached, &io, nullptr, 4096, 1));
    EXPECT_FALSE(jpeg_buffered_source_init(&cached, &io, buffer, 0, 1));
    EXPECT_FALSE(jpeg_buffered_source_init(&cached, &io, buffer, 4096, 0));
}
} // namespace
