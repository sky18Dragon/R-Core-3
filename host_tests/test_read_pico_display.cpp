#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <functional>
#include <vector>

#include "read_pico_frame.h"
#include "read_pico_refresh.h"

extern "C" {
#include "GUI_Paint.h"
}

namespace
{
constexpr read_pico_frame_layout_t kSmall{4, 2, 2};
constexpr read_pico_frame_layout_t kPanel{1216, 684, 608};
const std::array<uint8_t, 4> kSource{0x01, 0x23, 0x45, 0x67};

uint8_t ReadLow(const std::vector<uint8_t> &frame, size_t stride, size_t x, size_t y)
{
    uint8_t byte = frame[y * stride + x / 2];
    return x % 2 ? byte >> 4 : byte & 15;
}

TEST(ReadPicoFrame, AllSixteenLevelsSwapNibblesWithoutInverting)
{
    const std::array<uint8_t, 8> source{0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    std::array<uint8_t, 8> out{};
    read_pico_frame_layout_t layout{16, 1, 8};
    ASSERT_EQ(read_pico_frame_convert(source.data(), 8, layout, out.data(), 8, layout,
                                      READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 8>{0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xdc, 0xfe}));
}

TEST(ReadPicoFrame, Rotation90IsClockwise)
{
    std::array<uint8_t, 4> out{};
    ASSERT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, {2, 4, 1},
                                      READ_PICO_ROTATE_90, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0x04, 0x15, 0x26, 0x37}));
}

TEST(ReadPicoFrame, Rotation180PreservesFourCorners)
{
    std::array<uint8_t, 4> out{};
    ASSERT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, kSmall,
                                      READ_PICO_ROTATE_180, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0x67, 0x45, 0x23, 0x01}));
}

TEST(ReadPicoFrame, Rotation270PreservesFourCorners)
{
    std::array<uint8_t, 4> out{};
    ASSERT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, {2, 4, 1},
                                      READ_PICO_ROTATE_270, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0x73, 0x62, 0x51, 0x40}));
}

TEST(ReadPicoFrame, InversionMustBeExplicit)
{
    std::array<uint8_t, 4> out{};
    ASSERT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, kSmall,
                                      READ_PICO_ROTATE_0, true),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0xef, 0xcd, 0xab, 0x89}));
}

TEST(ReadPicoFrame, OddRowsIgnoreSourcePaddingAndWhitenUnusedDestinationNibble)
{
    const std::array<uint8_t, 4> source{0x01, 0x2e, 0x34, 0x5d};
    std::array<uint8_t, 4> out{};
    ASSERT_EQ(read_pico_frame_convert(source.data(), 4, {3, 2, 2}, out.data(), 4, {3, 2, 2},
                                      READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0x10, 0xf2, 0x43, 0xf5}));
}

TEST(ReadPicoFrame, IndependentRowStridesLeavePaddingAndGuardsUntouched)
{
    const std::array<uint8_t, 6> source{0x01, 0x23, 0xee, 0x45, 0x67, 0xdd};
    std::array<uint8_t, 9> out;
    out.fill(0xa5);
    ASSERT_EQ(read_pico_frame_convert(source.data(), 5, {4, 2, 3}, out.data() + 1, 6, {4, 2, 4},
                                      READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, (std::array<uint8_t, 9>{0xa5, 0x10, 0x32, 0xa5, 0xa5, 0x54, 0x76, 0xa5, 0xa5}));
}

TEST(ReadPicoFrame, RejectsShortBuffersBeforeAnyWrite)
{
    std::array<uint8_t, 4> out;
    out.fill(0xa5);
    for (bool short_source : {false, true}) {
        EXPECT_EQ(read_pico_frame_convert(kSource.data(), short_source ? 3 : 4, kSmall, out.data(),
                                          short_source ? 4 : 3, kSmall, READ_PICO_ROTATE_0, false),
                  READ_PICO_FRAME_BUFFER_TOO_SMALL);
        EXPECT_EQ(out, (std::array<uint8_t, 4>{0xa5, 0xa5, 0xa5, 0xa5}));
    }
}

TEST(ReadPicoFrame, RejectsNullBadRotationAndMismatchedGeometry)
{
    std::array<uint8_t, 4> out{};
    EXPECT_EQ(read_pico_frame_convert(nullptr, 4, kSmall, out.data(), 4, kSmall, READ_PICO_ROTATE_0,
                                      false),
              READ_PICO_FRAME_INVALID_ARGUMENT);
    EXPECT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, nullptr, 4, kSmall,
                                      READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_INVALID_ARGUMENT);
    EXPECT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, kSmall,
                                      static_cast<read_pico_rotation_t>(4), false),
              READ_PICO_FRAME_INVALID_ARGUMENT);
    EXPECT_EQ(read_pico_frame_convert(kSource.data(), 4, kSmall, out.data(), 4, kSmall,
                                      READ_PICO_ROTATE_90, false),
              READ_PICO_FRAME_INVALID_GEOMETRY);
}

TEST(ReadPicoFrame, RejectsZeroDimensionsBadStrideAndOverflow)
{
    std::array<uint8_t, 4> out;
    out.fill(0xa5);
    for (read_pico_frame_layout_t bad : std::array<read_pico_frame_layout_t, 4>{
             {{0, 2, 2}, {4, 0, 2}, {4, 2, 1}, {4, 3, SIZE_MAX}}}) {
        EXPECT_EQ(read_pico_frame_convert(kSource.data(), SIZE_MAX, bad, out.data(), 4, kSmall,
                                          READ_PICO_ROTATE_0, false),
                  READ_PICO_FRAME_INVALID_GEOMETRY);
    }
    EXPECT_EQ(out, (std::array<uint8_t, 4>{0xa5, 0xa5, 0xa5, 0xa5}));
}

TEST(ReadPicoFrame, RejectsIdenticalAndPartiallyOverlappingSpans)
{
    std::array<uint8_t, 8> frame{};
    for (size_t offset : {0u, 1u, 3u}) {
        EXPECT_EQ(read_pico_frame_convert(frame.data(), 4, kSmall, frame.data() + offset, 4, kSmall,
                                          READ_PICO_ROTATE_0, false),
                  READ_PICO_FRAME_OVERLAP);
        EXPECT_EQ(read_pico_frame_convert(frame.data() + offset, 4, kSmall, frame.data(), 4, kSmall,
                                          READ_PICO_ROTATE_0, false),
                  READ_PICO_FRAME_OVERLAP);
    }
    EXPECT_EQ(read_pico_frame_convert(frame.data(), 4, kSmall, frame.data() + 4, 4, kSmall,
                                      READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
}

TEST(ReadPicoFrame, NativePanelEveryPixelAndSourceImmutability)
{
    std::vector<uint8_t> source(READ_PICO_FRAME_BYTES);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; x += 2)
            source[y * 608 + x / 2] =
                static_cast<uint8_t>((((x + y * 7) % 16) << 4) | ((x + 1 + y * 7) % 16));
    const auto original = source;
    std::vector<uint8_t> out(READ_PICO_FRAME_BYTES + 1, 0xa5);
    ASSERT_EQ(read_pico_frame_convert(source.data(), source.size(), kPanel, out.data(),
                                      READ_PICO_FRAME_BYTES, kPanel, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; ++x)
            ASSERT_EQ(ReadLow(out, 608, x, y), (x + y * 7) % 16);
    EXPECT_EQ(source, original);
    EXPECT_EQ(out.back(), 0xa5);
}

TEST(ReadPicoFrame, OddOnePixelAndRotatedOddDimensions)
{
    const uint8_t source = 0x3a;
    uint8_t out = 0;
    EXPECT_EQ(read_pico_frame_convert(&source, 1, {1, 1, 1}, &out, 1, {1, 1, 1},
                                      READ_PICO_ROTATE_270, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(out, 0xf3);
    const std::array<uint8_t, 4> odd{0x01, 0x2e, 0x34, 0x5d};
    std::array<uint8_t, 3> rotated{};
    ASSERT_EQ(read_pico_frame_convert(odd.data(), 4, {3, 2, 2}, rotated.data(), 3, {2, 3, 1},
                                      READ_PICO_ROTATE_90, false),
              READ_PICO_FRAME_OK);
    EXPECT_EQ(rotated, (std::array<uint8_t, 3>{0x03, 0x14, 0x25}));
}

TEST(ReadPicoFrame, ActualPhotoFramePainterProducesTheExpectedInputFormat)
{
    std::vector<uint8_t> source(READ_PICO_FRAME_BYTES, 0);
    Paint_NewImage(source.data(), 1216, 684, ROTATE_0, 15);
    Paint_SetScale(16);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; ++x)
            Paint_SetPixel(x, y, (x + y * 7) % 16);
    EXPECT_EQ(source[0], 0x01);
    std::vector<uint8_t> out(READ_PICO_FRAME_BYTES);
    ASSERT_EQ(read_pico_frame_convert(source.data(), source.size(), kPanel, out.data(), out.size(),
                                      kPanel, READ_PICO_ROTATE_0, false),
              READ_PICO_FRAME_OK);
    for (size_t y = 0; y < 684; ++y)
        for (size_t x = 0; x < 1216; ++x)
            ASSERT_EQ(ReadLow(out, 608, x, y), (x + y * 7) % 16);
    Paint.Image = nullptr;
}

struct Transport {
    bool locked = false;
    bool busy = false;
    int prepare_error = 0;
    int draw_error = 0;
    int stop_error = 0;
    int sample_error = 0, sampled_temperature = 25, expected_temperature = 25;
    bool write_sample = true;
    std::vector<int> calls;
    std::function<void()> on_draw;
    read_pico_refresh_ops_t ops{this,
                                [](void *p) {
                                    auto &t = *static_cast<Transport *>(p);
                                    t.calls.push_back(1);
                                    if (t.busy || t.locked)
                                        return false;
                                    t.locked = true;
                                    return true;
                                },
                                [](void *p) {
                                    auto &t = *static_cast<Transport *>(p);
                                    EXPECT_TRUE(t.locked);
                                    t.calls.push_back(5);
                                    t.locked = false;
                                },
                                [](void *p, int mv) {
                                    auto &t = *static_cast<Transport *>(p);
                                    EXPECT_TRUE(t.locked);
                                    EXPECT_EQ(mv, 1290);
                                    t.calls.push_back(2);
                                    return t.prepare_error;
                                },
                                [](void *p, const uint8_t *frame, size_t size, int temperature) {
                                    auto &t = *static_cast<Transport *>(p);
                                    EXPECT_TRUE(t.locked);
                                    EXPECT_EQ(frame[0], 0x10);
                                    EXPECT_EQ(frame[size - 1], 0xfe);
                                    EXPECT_EQ(size, READ_PICO_FRAME_BYTES);
                                    EXPECT_EQ(temperature, t.expected_temperature);
                                    t.calls.push_back(3);
                                    if (t.on_draw)
                                        t.on_draw();
                                    return t.draw_error;
                                },
                                [](void *p) {
                                    auto &t = *static_cast<Transport *>(p);
                                    EXPECT_TRUE(t.locked);
                                    t.calls.push_back(4);
                                    return t.stop_error;
                                }};
    void active_sensor()
    {
        ops.read_temperature = [](void *p, int *temperature) {
            auto &t = *static_cast<Transport *>(p);
            EXPECT_TRUE(t.locked);
            t.calls.push_back(6);
            if (t.write_sample)
                *temperature = t.sampled_temperature;
            return t.sample_error;
        };
    }
};

class ReadPicoRefresh : public testing::Test
{
   protected:
    Transport transport;
    std::vector<uint8_t> source = std::vector<uint8_t>(READ_PICO_FRAME_BYTES, 0x01);
    std::vector<uint8_t> scratch = std::vector<uint8_t>(READ_PICO_FRAME_BYTES, 0xa5);
    read_pico_refresh_gate_t gate{true, 1290, true, 25};

    void SetUp() override
    {
        source.back() = 0xef;
    }
    read_pico_refresh_result_t Run()
    {
        return read_pico_refresh_gc16(&transport.ops, gate, source.data(), source.size(),
                                      scratch.data(), scratch.size());
    }
};

TEST_F(ReadPicoRefresh, SuccessRequiresDrawAndCleanupUnderOneLock)
{
    auto original = source;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_OK);
    EXPECT_TRUE(result.completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(source, original);
}

TEST_F(ReadPicoRefresh, BusyDoesNotTouchBuffersOrHardware)
{
    transport.busy = true;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_BUSY);
    EXPECT_FALSE(result.completed);
    EXPECT_EQ(scratch.front(), 0xa5);
    EXPECT_EQ(transport.calls, (std::vector<int>{1}));
}

TEST_F(ReadPicoRefresh, MissingCalibrationDoesNotEnergizePanel)
{
    gate.vcom_valid = false;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_GATE_CLOSED);
    EXPECT_FALSE(result.completed);
    EXPECT_EQ(scratch.front(), 0xa5);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5}));
}

TEST_F(ReadPicoRefresh, InvalidVcomRangeAndStepCloseGate)
{
    for (int mv : {499, 2501, 1291, -1}) {
        gate.vcom_mv = mv;
        EXPECT_EQ(Run().error, READ_PICO_REFRESH_GATE_CLOSED);
    }
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5, 1, 5, 1, 5, 1, 5}));
}

TEST_F(ReadPicoRefresh, UnknownOrOutOfWaveformRangeTemperatureClosesGate)
{
    gate.temperature_valid = false;
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_GATE_CLOSED);
    gate.temperature_valid = true;
    for (int temperature : {-1, 51}) {
        gate.temperature_c = temperature;
        EXPECT_EQ(Run().error, READ_PICO_REFRESH_GATE_CLOSED);
    }
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5, 1, 5, 1, 5}));
}

TEST_F(ReadPicoRefresh, WrongLengthAndShortScratchNeverCallPrepare)
{
    source.pop_back();
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_BAD_FRAME);
    source.resize(READ_PICO_FRAME_BYTES + 1);
    EXPECT_EQ(Run().frame_error, READ_PICO_FRAME_INVALID_ARGUMENT);
    source.resize(READ_PICO_FRAME_BYTES);
    scratch.pop_back();
    EXPECT_EQ(Run().frame_error, READ_PICO_FRAME_BUFFER_TOO_SMALL);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5, 1, 5, 1, 5}));
}

TEST_F(ReadPicoRefresh, OverlapRejectedBeforePowerOn)
{
    auto result = read_pico_refresh_gc16(&transport.ops, gate, source.data(), source.size(),
                                         source.data(), source.size());
    EXPECT_EQ(result.error, READ_PICO_REFRESH_BAD_FRAME);
    EXPECT_EQ(result.frame_error, READ_PICO_FRAME_OVERLAP);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5}));
}

TEST_F(ReadPicoRefresh, PartialPrepareFailureStillStopsAndReleases)
{
    transport.prepare_error = 17;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_PREPARE_FAILED);
    EXPECT_EQ(result.operation_error, 17);
    EXPECT_FALSE(result.completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 4, 5}));
}

TEST_F(ReadPicoRefresh, DrawFailureIsNotSuccessAndStillCleansUp)
{
    transport.draw_error = 23;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_DRAW_FAILED);
    EXPECT_EQ(result.operation_error, 23);
    EXPECT_FALSE(result.completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST_F(ReadPicoRefresh, CleanupFailureDoesNotReportCompletedFrame)
{
    transport.stop_error = 29;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_STOP_FAILED);
    EXPECT_EQ(result.cleanup_error, 29);
    EXPECT_FALSE(result.completed);
    EXPECT_FALSE(transport.locked);
}

TEST_F(ReadPicoRefresh, PreservesBothPrimaryAndCleanupFailure)
{
    transport.prepare_error = 17;
    transport.stop_error = 29;
    auto result = Run();
    EXPECT_EQ(result.error, READ_PICO_REFRESH_PREPARE_FAILED);
    EXPECT_EQ(result.operation_error, 17);
    EXPECT_EQ(result.cleanup_error, 29);
    EXPECT_FALSE(result.completed);
}

TEST_F(ReadPicoRefresh, MissingCallbacksNeverAcquire)
{
    EXPECT_EQ(read_pico_refresh_gc16(nullptr, gate, source.data(), source.size(), scratch.data(),
                                     scratch.size())
                  .error,
              READ_PICO_REFRESH_INVALID_ARGUMENT);
    transport.ops.draw_gc16 = nullptr;
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_INVALID_ARGUMENT);
    EXPECT_TRUE(transport.calls.empty());
}

TEST_F(ReadPicoRefresh, ReentrantRefreshIsRejectedWithoutMutatingActiveFrame)
{
    transport.on_draw = [this]() {
        const auto before = scratch;
        auto nested = Run();
        EXPECT_EQ(nested.error, READ_PICO_REFRESH_BUSY);
        EXPECT_FALSE(nested.completed);
        EXPECT_EQ(scratch, before);
    };
    EXPECT_TRUE(Run().completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 3, 1, 4, 5}));
}

TEST_F(ReadPicoRefresh, ActiveSensorSamplesAfterPrepareAndIgnoresStandbyGateValue)
{
    transport.active_sensor();
    gate.temperature_valid = false;
    gate.temperature_c = -100;
    transport.sampled_temperature = transport.expected_temperature = 31;
    EXPECT_TRUE(Run().completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 6, 3, 4, 5}));
}

TEST_F(ReadPicoRefresh, ActiveSensorStillCannotBypassVcomOrFrameChecks)
{
    transport.active_sensor();
    gate.vcom_valid = false;
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_GATE_CLOSED);
    gate.vcom_valid = true;
    source.pop_back();
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_BAD_FRAME);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 5, 1, 5}));
}

TEST_F(ReadPicoRefresh, FailedPrepareNeverSamplesActiveSensor)
{
    transport.active_sensor();
    transport.prepare_error = 17;
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_PREPARE_FAILED);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 4, 5}));
}

TEST_F(ReadPicoRefresh, ActiveSensorErrorForbidsDrawAndPreservesCleanupError)
{
    transport.active_sensor();
    transport.sample_error = 37;
    transport.stop_error = 29;
    auto r = Run();
    EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
    EXPECT_EQ(r.operation_error, 37);
    EXPECT_EQ(r.cleanup_error, 29);
    EXPECT_FALSE(r.completed);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 6, 4, 5}));
    EXPECT_FALSE(transport.locked);
}

TEST_F(ReadPicoRefresh, ActiveSensorOutOfRangeForbidsDrawEvenWithValidCachedTemperature)
{
    transport.active_sensor();
    for (int value : {-1, 51}) {
        transport.calls.clear();
        transport.sampled_temperature = value;
        auto r = Run();
        EXPECT_EQ(r.error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
        EXPECT_FALSE(r.completed);
        EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 6, 4, 5}));
    }
}

TEST_F(ReadPicoRefresh, ActiveSensorSuccessWithoutSampleIsRejected)
{
    transport.active_sensor();
    transport.write_sample = false;
    EXPECT_EQ(Run().error, READ_PICO_REFRESH_TEMPERATURE_FAILED);
    EXPECT_EQ(transport.calls, (std::vector<int>{1, 2, 6, 4, 5}));
}

TEST_F(ReadPicoRefresh, ValidActiveZeroAndFiftyAreNotArbitrarilyRejected)
{
    transport.active_sensor();
    for (int value : {0, 50}) {
        transport.sampled_temperature = transport.expected_temperature = value;
        EXPECT_TRUE(Run().completed);
    }
}
}  // namespace
