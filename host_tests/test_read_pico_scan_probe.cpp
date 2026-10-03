#include <gtest/gtest.h>

#include <array>
#include <cstring>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "output_lcd/lcd_driver.h"
#include "read_pico_scan_probe.h"
void __wrap_epd_lcd_line_source_cb(line_cb_func_t, void *);
void __wrap_epd_lcd_frame_done_cb(frame_done_func_t, void *);
void __wrap_epd_lcd_start_frame(void);
}

namespace
{
line_cb_func_t installed_line;
frame_done_func_t installed_done;
void *installed_line_arg, *installed_done_arg;
int core, critical_depth;
unsigned original_lines, original_done;
bool original_return, finish_inside_line, duplicate_edge_group, break_edge_pair;
int line_token, done_token;
int64_t clock_us;
unsigned actual_starts;

bool source(void *arg, uint8_t *bytes)
{
    EXPECT_EQ(critical_depth, 0);
    EXPECT_EQ(arg, &line_token);
    unsigned phase = original_done % 26;
    uint8_t pulse = phase < 10 ? 0x55 : phase < 23 ? 0xaa : 0x00;
    std::memset(bytes, original_lines < 684 ? pulse : 0x00, 304);
    ++original_lines;
    if (finish_inside_line && installed_done)
        installed_done(installed_done_arg);
    return original_return;
}
unsigned edge_group(unsigned row)
{
    if (row == 683 || row == 644)
        return 0;
    if (row == 643 || row == 607)
        return break_edge_pair && row == 607 ? 6 : 1;
    if (row == 606 || row == 604)
        return 2;
    if (row == 603 || row == 570)
        return duplicate_edge_group ? 2 : 3;
    if (row == 569 || row == 564)
        return 4;
    if (row == 563 || row == 556)
        return 5;
    return 7;
}
bool edge_source(void *arg, uint8_t *bytes)
{
    EXPECT_EQ(critical_depth, 0);
    EXPECT_EQ(arg, &line_token);
    unsigned group = edge_group(original_lines++);
    std::memset(bytes, (group * 31u + original_done * 7u) & 0xffu, 304);
    return original_return;
}
void done(void *arg)
{
    EXPECT_EQ(critical_depth, 0);
    EXPECT_EQ(arg, &done_token);
    ++original_done;
    __wrap_epd_lcd_frame_done_cb(nullptr, nullptr);
    __wrap_epd_lcd_line_source_cb(nullptr, nullptr);
}
class ReadPicoScanProbe : public testing::Test
{
   protected:
    void SetUp() override
    {
        core = critical_depth = 0;
        clock_us = 100;
        actual_starts = 0;
        read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
        read_pico_phase_probe_end(36);
        finish_inside_line = original_return = duplicate_edge_group = break_edge_pair = false;
        __wrap_epd_lcd_frame_done_cb(nullptr, nullptr);
        __wrap_epd_lcd_line_source_cb(nullptr, nullptr);
        read_pico_scan_probe_end();
        original_lines = original_done = 0;
    }
    void TearDown() override
    {
        finish_inside_line = false;
        read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
        __wrap_epd_lcd_frame_done_cb(nullptr, nullptr);
        __wrap_epd_lcd_line_source_cb(nullptr, nullptr);
        read_pico_scan_probe_end();
        EXPECT_EQ(critical_depth, 0);
    }
    void gc16_frame(unsigned count = READ_PICO_EDGE_EXPECTED_SOURCE_CALLS,
                    bool start_on_core1 = false)
    {
        original_lines = 0;
        __wrap_epd_lcd_frame_done_cb(done, &done_token);
        __wrap_epd_lcd_line_source_cb(edge_source, &line_token);
        clock_us += 200;
        __wrap_epd_lcd_start_frame();
        std::array<uint8_t, 304> bytes{};
        for (unsigned i = 0; i < count; ++i) {
            core = start_on_core1 && i < READ_PICO_EDGE_START_PREFILL_CALLS ? 1 : 0;
            EXPECT_EQ(installed_line(installed_line_arg, bytes.data()), original_return);
        }
        core = 0;
        clock_us += 11216;
        installed_done(installed_done_arg);
        clock_us += 30;
    }
    void frame(unsigned count = 688)
    {
        original_lines = 0;
        __wrap_epd_lcd_frame_done_cb(done, &done_token);
        __wrap_epd_lcd_line_source_cb(source, &line_token);
        std::array<uint8_t, 306> guarded{};
        guarded.front() = guarded.back() = 0x7b;
        for (unsigned i = 0; i < count; ++i)
            EXPECT_EQ(installed_line(installed_line_arg, guarded.data() + 1), original_return);
        EXPECT_EQ(guarded.front(), 0x7b);
        EXPECT_EQ(guarded.back(), 0x7b);
        installed_done(installed_done_arg);
    }
};
}  // namespace

extern "C" {
int64_t esp_timer_get_time(void)
{
    return clock_us;
}
void __real_epd_lcd_start_frame(void)
{
    EXPECT_EQ(critical_depth, 0);
    ++actual_starts;
}
void probe_test_enter(portMUX_TYPE *)
{
    EXPECT_EQ(critical_depth++, 0);
}
void probe_test_exit(portMUX_TYPE *)
{
    EXPECT_EQ(--critical_depth, 0);
}
int xPortGetCoreID(void)
{
    return core;
}
void __real_epd_lcd_line_source_cb(line_cb_func_t fn, void *arg)
{
    installed_line = fn;
    installed_line_arg = arg;
}
void __real_epd_lcd_frame_done_cb(frame_done_func_t fn, void *arg)
{
    installed_done = fn;
    installed_done_arg = arg;
}
}

TEST_F(ReadPicoScanProbe, DisabledForwardsExactCallbacksAndPayloads)
{
    __wrap_epd_lcd_line_source_cb(source, &line_token);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    EXPECT_EQ(installed_line, source);
    EXPECT_EQ(installed_line_arg, &line_token);
    EXPECT_EQ(installed_done, done);
    EXPECT_EQ(installed_done_arg, &done_token);
    EXPECT_EQ(read_pico_scan_probe_begin(), ESP_ERR_INVALID_STATE);
}

TEST_F(ReadPicoScanProbe, PhaseTimingPreservesLineCallbackAndMeasuresAll36Frames)
{
    ASSERT_EQ(read_pico_phase_probe_begin(), ESP_OK);
    EXPECT_EQ(read_pico_scan_probe_begin(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_phase_probe_begin(), ESP_ERR_INVALID_STATE);
    for (int i = 0; i < 36; ++i) {
        __wrap_epd_lcd_frame_done_cb(done, &done_token);
        __wrap_epd_lcd_line_source_cb(source, &line_token);
        EXPECT_EQ(installed_line, source);
        clock_us += 200;
        __wrap_epd_lcd_start_frame();
        clock_us += 11216;
        installed_done(installed_done_arg);
        clock_us += 30;
    }
    const auto *p = read_pico_phase_probe_end(36);
    EXPECT_TRUE(p->complete);
    EXPECT_EQ(actual_starts, 36u);
    EXPECT_EQ(original_done, 36u);
    for (unsigned i = 0; i < 36; ++i) {
        EXPECT_EQ(p->phases[i].start_us - p->phases[i].registered_us, 200);
        EXPECT_EQ(p->phases[i].done_us - p->phases[i].start_us, 11216);
        if (i)
            EXPECT_EQ(p->phases[i].registered_us - p->phases[i - 1].done_us, 30);
    }
    auto frozen = *p;
    __wrap_epd_lcd_start_frame();
    EXPECT_EQ(std::memcmp(p, &frozen, sizeof(frozen)), 0);
}

TEST_F(ReadPicoScanProbe, Gray16EdgeProbeDistinguishesExpectedLogicalRowBoundaries)
{
    ASSERT_EQ(read_pico_gc16_probe_begin(), ESP_OK);
    EXPECT_EQ(read_pico_scan_probe_begin(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_phase_probe_begin(), ESP_ERR_INVALID_STATE);
    for (int i = 0; i < READ_PICO_EDGE_EXPECTED_FRAMES; ++i)
        gc16_frame(READ_PICO_EDGE_EXPECTED_SOURCE_CALLS, i < 32);
    const auto *capture = read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
    read_pico_edge_summary_t summary{};
    ASSERT_TRUE(read_pico_edge_analyze(&capture->lines, &summary));
    EXPECT_TRUE(summary.software_complete);
    EXPECT_TRUE(capture->phases.complete);
    EXPECT_EQ(summary.min_source_calls, READ_PICO_EDGE_EXPECTED_SOURCE_CALLS);
    EXPECT_EQ(summary.max_source_calls, READ_PICO_EDGE_EXPECTED_SOURCE_CALLS);
    EXPECT_EQ(capture->lines.core1_start_prefill, 32u * READ_PICO_EDGE_START_PREFILL_CALLS);
    EXPECT_EQ(capture->lines.wrong_core, 0u);
    EXPECT_EQ(summary.indistinguishable_boundaries, 0);
    for (unsigned pair = 0; pair < READ_PICO_EDGE_TRACE_ROWS / 2; ++pair)
        EXPECT_EQ(summary.trajectory_hashes[pair * 2],
                  summary.trajectory_hashes[pair * 2 + 1]);
    for (unsigned pair = 0; pair + 1 < READ_PICO_EDGE_TRACE_ROWS / 2; ++pair)
        EXPECT_NE(summary.trajectory_hashes[pair * 2],
                  summary.trajectory_hashes[(pair + 1) * 2]);
    EXPECT_EQ(read_pico_edge_trace_row(0), 683);
    EXPECT_EQ(read_pico_edge_trace_row(11), 556);
    EXPECT_EQ(read_pico_edge_trace_row(12), UINT16_MAX);
    EXPECT_EQ(installed_line, nullptr);
    EXPECT_EQ(installed_done, nullptr);
}

TEST_F(ReadPicoScanProbe, Gray16EdgeProbeFailsClosedForDuplicateOrUnequalRows)
{
    duplicate_edge_group = true;
    ASSERT_EQ(read_pico_gc16_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_EDGE_EXPECTED_FRAMES; ++i)
        gc16_frame();
    const auto *duplicate = read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
    read_pico_edge_summary_t summary{};
    ASSERT_TRUE(read_pico_edge_analyze(&duplicate->lines, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(summary.indistinguishable_boundaries, 1u << 2);

    duplicate_edge_group = false;
    break_edge_pair = true;
    ASSERT_EQ(read_pico_gc16_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_EDGE_EXPECTED_FRAMES; ++i)
        gc16_frame();
    const auto *unequal = read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
    ASSERT_TRUE(read_pico_edge_analyze(&unequal->lines, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(summary.unequal_pair_frames, READ_PICO_EDGE_EXPECTED_FRAMES);
}

TEST_F(ReadPicoScanProbe, Gray16EdgeProbeRejectsOld688CountAndLateCore1Callbacks)
{
    ASSERT_EQ(read_pico_gc16_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_EDGE_EXPECTED_FRAMES; ++i)
        gc16_frame(688);
    const auto *short_capture = read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
    read_pico_edge_summary_t summary{};
    ASSERT_TRUE(read_pico_edge_analyze(&short_capture->lines, &summary));
    EXPECT_FALSE(summary.software_complete);

    ASSERT_EQ(read_pico_gc16_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_EDGE_EXPECTED_FRAMES; ++i) {
        original_lines = 0;
        __wrap_epd_lcd_frame_done_cb(done, &done_token);
        __wrap_epd_lcd_line_source_cb(edge_source, &line_token);
        __wrap_epd_lcd_start_frame();
        std::array<uint8_t, 304> bytes{};
        for (unsigned row = 0; row < READ_PICO_EDGE_EXPECTED_SOURCE_CALLS; ++row) {
            core = row == 100 ? 1 : 0;
            installed_line(installed_line_arg, bytes.data());
        }
        core = 0;
        installed_done(installed_done_arg);
    }
    const auto *wrong_core_capture = read_pico_gc16_probe_end(READ_PICO_EDGE_EXPECTED_FRAMES);
    ASSERT_TRUE(read_pico_edge_analyze(&wrong_core_capture->lines, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(wrong_core_capture->lines.wrong_core, READ_PICO_EDGE_EXPECTED_FRAMES);
}

TEST_F(ReadPicoScanProbe, PhaseMissingOrDuplicateStartsAndIncompleteFramesFailClosed)
{
    ASSERT_EQ(read_pico_phase_probe_begin(), ESP_OK);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    installed_done(installed_done_arg);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    __wrap_epd_lcd_start_frame();
    __wrap_epd_lcd_start_frame();
    const auto *p = read_pico_phase_probe_end(2);
    EXPECT_FALSE(p->complete);
    EXPECT_GE(p->errors, 3u);
    EXPECT_EQ(installed_done, done);
    __wrap_epd_lcd_frame_done_cb(nullptr, nullptr);
}

TEST_F(ReadPicoScanProbe, PhaseOverflowIsBoundedAndCountMismatchCannotPass)
{
    ASSERT_EQ(read_pico_phase_probe_begin(), ESP_OK);
    for (int i = 0; i < 50; ++i) {
        __wrap_epd_lcd_frame_done_cb(done, &done_token);
        __wrap_epd_lcd_start_frame();
        ++clock_us;
        installed_done(installed_done_arg);
    }
    const auto *p = read_pico_phase_probe_end(36);
    EXPECT_FALSE(p->complete);
    EXPECT_EQ(p->stored, 48u);
    EXPECT_EQ(p->dropped, 2u);
    EXPECT_EQ(actual_starts, 50u);
    EXPECT_EQ(original_done, 50u);
}
TEST_F(ReadPicoScanProbe, FullStaticClearRecordsTailAndPreservesReturnValues)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    original_return = true;
    for (int i = 0; i < READ_PICO_CLEAR_EXPECTED_FRAMES; ++i)
        frame();
    const auto *capture = read_pico_scan_probe_end();
    read_pico_scan_summary_t summary{};
    ASSERT_TRUE(read_pico_scan_analyze(capture, &summary));
    EXPECT_TRUE(summary.software_complete);
    EXPECT_EQ(original_done, 78u);
    EXPECT_EQ(summary.min_source_calls, 688u);
    EXPECT_EQ(summary.max_source_calls, 688u);
    EXPECT_EQ(capture->frames[0].seen_rows, 0xff);
    EXPECT_EQ(capture->frames[0].first[3], 0x55);
    EXPECT_EQ(capture->frames[0].last[3], 0x55);
    EXPECT_EQ(capture->frames[0].first[4], 0);
    EXPECT_EQ(installed_line, nullptr);
    EXPECT_EQ(installed_done, nullptr);
}
TEST_F(ReadPicoScanProbe, RejectsNestedBeginAndWrongOwnerCore)
{
    core = 1;
    EXPECT_EQ(read_pico_scan_probe_begin(), ESP_ERR_INVALID_STATE);
    core = 0;
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    EXPECT_EQ(read_pico_scan_probe_begin(), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoScanProbe, ShortFrameAndUnfinishedRegistrationCannotProveCoverage)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    frame(680);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    const auto *capture = read_pico_scan_probe_end();
    read_pico_scan_summary_t summary{};
    ASSERT_TRUE(read_pico_scan_analyze(capture, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(summary.missing_tail_frames, 1u);
    EXPECT_EQ(summary.missing_padding_frames, 1u);
    EXPECT_EQ(capture->unfinished_frames, 1u);
    EXPECT_EQ(installed_done, done);
    EXPECT_EQ(installed_done_arg, &done_token);
}
TEST_F(ReadPicoScanProbe, OverflowIsBoundedAndCannotBeReportedComplete)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_TRACE_MAX_FRAMES + 2; ++i)
        frame(0);
    const auto *capture = read_pico_scan_probe_end();
    EXPECT_EQ(capture->stored_frames, 96u);
    EXPECT_EQ(capture->dropped_frames, 2u);
    EXPECT_EQ(original_done, 98u);
}
TEST_F(ReadPicoScanProbe, FrameDoneDuringSourceIsRecordedAsLateNotAnotherFrame)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    __wrap_epd_lcd_line_source_cb(source, &line_token);
    finish_inside_line = true;
    std::array<uint8_t, 304> bytes{};
    installed_line(installed_line_arg, bytes.data());
    const auto *capture = read_pico_scan_probe_end();
    EXPECT_EQ(capture->late_lines, 1u);
    EXPECT_EQ(capture->unfinished_frames, 1u);
    EXPECT_EQ(capture->frames[0].seen_rows, 0);
}
TEST_F(ReadPicoScanProbe, WrongCoreAndOrphanCallsAreNotHidden)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    __wrap_epd_lcd_line_source_cb(source, &line_token);
    core = 1;
    std::array<uint8_t, 304> bytes{};
    installed_line(installed_line_arg, bytes.data());
    core = 0;
    const auto *capture = read_pico_scan_probe_end();
    EXPECT_EQ(capture->orphan_lines, 1u);
    EXPECT_EQ(capture->wrong_core, 1u);
    EXPECT_EQ(original_lines, 1u);
    EXPECT_EQ(installed_line, source);
    EXPECT_EQ(installed_line_arg, &line_token);
}
TEST_F(ReadPicoScanProbe, FrozenResultIsNotMutatedByLaterRegistrations)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    frame();
    const auto *capture = read_pico_scan_probe_end();
    auto before = *capture;
    frame(680);
    EXPECT_EQ(std::memcmp(capture, &before, sizeof(before)), 0);
}

TEST(ReadPicoScanAnalysis, InvalidArgumentsLeaveOutputUnchanged)
{
    read_pico_scan_capture_t capture{};
    read_pico_scan_summary_t summary{};
    summary.max_source_calls = 123;
    EXPECT_FALSE(read_pico_scan_analyze(nullptr, &summary));
    EXPECT_FALSE(read_pico_scan_analyze(&capture, nullptr));
    capture.stored_frames = READ_PICO_TRACE_MAX_FRAMES + 1;
    EXPECT_FALSE(read_pico_scan_analyze(&capture, &summary));
    EXPECT_EQ(summary.max_source_calls, 123u);
}
TEST(ReadPicoScanAnalysis, NonzeroPaddingUnequalEdgesAndMissingSamplesPreventCoverage)
{
    read_pico_scan_capture_t capture{};
    capture.registered_frames = capture.completed_frames = capture.stored_frames = 78;
    for (auto &frame : capture.frames) {
        frame.source_calls = 696;  // Callback prefetch is not physical transmission.
        frame.seen_rows = 0xff;
    }
    capture.frames[0].first[4] = 1;
    capture.frames[1].seen_rows = 0xf7;
    capture.frames[2].seen_rows = 0x0f;
    read_pico_scan_summary_t summary{};
    ASSERT_TRUE(read_pico_scan_analyze(&capture, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(summary.nonzero_padding_frames, 1u);
    EXPECT_EQ(summary.unequal_edge_frames, 1u);
    EXPECT_EQ(summary.missing_tail_frames, 1u);
    EXPECT_EQ(summary.missing_padding_frames, 1u);
}

TEST(ReadPicoScanAnalysis, MissingExtraFramesOrAnyRecorderFaultPreventCoverage)
{
    read_pico_scan_capture_t capture{};
    capture.registered_frames = capture.completed_frames = capture.stored_frames = 78;
    for (auto &frame : capture.frames) {
        frame.source_calls = 688;
        frame.seen_rows = 0xff;
        unsigned phase = (&frame - capture.frames) % 26;
        for (unsigned row = 0; row < 4; ++row)
            frame.first[row] = frame.last[row] = phase < 10 ? 0x55 : phase < 23 ? 0xaa : 0x00;
    }
    read_pico_scan_summary_t summary{};
    ASSERT_TRUE(read_pico_scan_analyze(&capture, &summary));
    EXPECT_TRUE(summary.software_complete);
    for (auto field : {&capture.dropped_frames, &capture.orphan_lines, &capture.late_lines,
                       &capture.unfinished_frames, &capture.wrong_core}) {
        *field = 1;
        ASSERT_TRUE(read_pico_scan_analyze(&capture, &summary));
        EXPECT_FALSE(summary.software_complete);
        *field = 0;
    }
    for (auto field :
         {&capture.registered_frames, &capture.completed_frames, &capture.stored_frames}) {
        for (uint32_t count : {77u, 79u}) {
            *field = count;
            ASSERT_TRUE(read_pico_scan_analyze(&capture, &summary));
            EXPECT_FALSE(summary.software_complete);
        }
        *field = 78;
    }
}

TEST_F(ReadPicoScanProbe, ReplacingAnOpenFrameRecordsFaultAndOnlyLatestSamples)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    __wrap_epd_lcd_frame_done_cb(done, &done_token);
    frame();
    const auto *capture = read_pico_scan_probe_end();
    EXPECT_EQ(capture->registered_frames, 2u);
    EXPECT_EQ(capture->completed_frames, 1u);
    EXPECT_EQ(capture->unfinished_frames, 1u);
    EXPECT_EQ(capture->frames[0].seen_rows, 0xff);
}

TEST_F(ReadPicoScanProbe, WhiteTailNoOpCannotPassJustBecauseAllCallbacksRan)
{
    ASSERT_EQ(read_pico_scan_probe_begin(), ESP_OK);
    for (int i = 0; i < READ_PICO_CLEAR_EXPECTED_FRAMES; ++i)
        frame();
    auto capture = *read_pico_scan_probe_end();
    for (unsigned row = 0; row < 4; ++row)
        capture.frames[10].first[row] = capture.frames[10].last[row] = 0;
    read_pico_scan_summary_t summary{};
    ASSERT_TRUE(read_pico_scan_analyze(&capture, &summary));
    EXPECT_FALSE(summary.software_complete);
    EXPECT_EQ(summary.unexpected_pulse_frames, 1u);
    EXPECT_EQ(summary.missing_tail_frames, 0u);
    EXPECT_EQ(summary.missing_padding_frames, 0u);
}
