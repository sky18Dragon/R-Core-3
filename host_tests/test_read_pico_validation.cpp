#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "read_pico_validation.h"
}

namespace
{
struct Fake {
    int clears = 0, confirmations = 0, stops = 0;
    int clear_error = 0, confirmation_error = 0, stop_error = 0;
    int draws = 0, draw_error = 0, continuous_draws = 0, continuous_error = 0;
    read_pico_pattern_t pattern = READ_PICO_PATTERN_COUNT;
};
int clear(void *p)
{
    auto &f = *static_cast<Fake *>(p);
    ++f.clears;
    return f.clear_error;
}
int confirm(void *p)
{
    auto &f = *static_cast<Fake *>(p);
    ++f.confirmations;
    return f.confirmation_error;
}
int stop(void *p)
{
    auto &f = *static_cast<Fake *>(p);
    ++f.stops;
    return f.stop_error;
}
int draw(void *p, read_pico_pattern_t pattern)
{
    auto &f = *static_cast<Fake *>(p);
    ++f.draws;
    f.pattern = pattern;
    return f.draw_error;
}
int draw_continuous(void *p)
{
    auto &f = *static_cast<Fake *>(p);
    ++f.continuous_draws;
    return f.continuous_error;
}
read_pico_validation_session_t begin(Fake &f)
{
    read_pico_validation_ops_t ops = {&f, clear, confirm, stop};
    read_pico_validation_session_t s{};
    EXPECT_TRUE(read_pico_validation_init(&s, &ops));
    return s;
}
read_pico_validation_command_t line(const std::string &text)
{
    read_pico_validation_reader_t r{};
    auto result = READ_PICO_VALIDATION_NONE;
    for (unsigned char c : text)
        result = read_pico_validation_feed(&r, c);
    return result;
}
}  // namespace

static read_pico_validation_session_t ready(Fake &f)
{
    auto s = begin(f);
    s.ops.draw_pattern = draw;
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    return s;
}

TEST(ReadPicoValidation, InitializationHasNoIoAndRejectsMissingCallbacks)
{
    Fake f;
    auto s = begin(f);
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_CLEAR);
    EXPECT_EQ(f.clears + f.confirmations + f.stops, 0);
    EXPECT_FALSE(read_pico_validation_init(nullptr, nullptr));
    read_pico_validation_ops_t ops = {&f, clear, nullptr, stop};
    EXPECT_FALSE(read_pico_validation_init(&s, &ops));
    EXPECT_FALSE(read_pico_validation_apply(nullptr, READ_PICO_VALIDATION_CLEAR_WHITE));
    read_pico_validation_session_t empty{};
    EXPECT_FALSE(read_pico_validation_apply(&empty, READ_PICO_VALIDATION_CLEAR_WHITE));
}

TEST(ReadPicoValidation, ConfirmationCannotPrecedeClear)
{
    Fake f;
    auto s = begin(f);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_EQ(f.confirmations, 0);
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_CLEAR);
}

TEST(ReadPicoValidation, ClearIsOneShotAndNeverAutomaticallyConfirms)
{
    Fake f;
    auto s = begin(f);
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_WHITE);
    EXPECT_EQ(f.clears, 1);
    EXPECT_EQ(f.confirmations, 0);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(f.clears, 1);
}

TEST(ReadPicoValidation, ExplicitConfirmationOnlyOnceAndNeverDraws)
{
    Fake f;
    auto s = begin(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_REFERENCE_KNOWN);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(f.clears, 1);
    EXPECT_EQ(f.confirmations, 1);
}

TEST(ReadPicoValidation, ClearFailureLatchesAndAllowsOnlyStatusOrExplicitStop)
{
    Fake f;
    f.clear_error = 77;
    auto s = begin(f);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_EQ(s.operation_error, 77);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STATUS));
    EXPECT_EQ(f.clears, 1);
    EXPECT_EQ(f.confirmations, 0);
    EXPECT_EQ(f.stops, 0);
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_STOPPED);
}

TEST(ReadPicoValidation, ConfirmationFailureNeverEstablishesReference)
{
    Fake f;
    f.confirmation_error = 78;
    auto s = begin(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_EQ(s.operation_error, 78);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_EQ(f.confirmations, 1);
}

TEST(ReadPicoValidation, CleanupFailureRequiresExplicitRetryAndStopIsTerminal)
{
    Fake f;
    f.stop_error = 79;
    auto s = begin(f);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    f.stop_error = 0;
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(f.stops, 2);
    EXPECT_EQ(f.clears, 0);
}

TEST(ReadPicoValidation, ParserRequiresExactKnownAsciiLine)
{
    EXPECT_EQ(line("clear-white\n"), READ_PICO_VALIDATION_CLEAR_WHITE);
    EXPECT_EQ(line("white-confirm\r"), READ_PICO_VALIDATION_CONFIRM_WHITE);
    EXPECT_EQ(line("stop\n"), READ_PICO_VALIDATION_STOP);
    EXPECT_EQ(line("status\n"), READ_PICO_VALIDATION_STATUS);
    for (const auto &bad : {"clear-white x\n", " clear-white\n", "CLEAR-WHITE\n", "clear-white\t\n",
                            "gc16\n", "clear-whit\n"})
        EXPECT_EQ(line(bad), READ_PICO_VALIDATION_INVALID);
    EXPECT_EQ(line(std::string("clear-white\0\n", 13)), READ_PICO_VALIDATION_INVALID);
    EXPECT_EQ(read_pico_validation_feed(nullptr, 'x'), READ_PICO_VALIDATION_INVALID);
}

TEST(ReadPicoValidation, OverflowDiscardsWholeLineAndRecoversOnlyAfterDelimiter)
{
    read_pico_validation_reader_t r{};
    for (unsigned char c : "clear-white" + std::string(100, 'x'))
        EXPECT_EQ(read_pico_validation_feed(&r, c), READ_PICO_VALIDATION_NONE);
    EXPECT_EQ(read_pico_validation_feed(&r, '\n'), READ_PICO_VALIDATION_INVALID);
    for (unsigned char c : std::string("status"))
        EXPECT_EQ(read_pico_validation_feed(&r, c), READ_PICO_VALIDATION_NONE);
    EXPECT_EQ(read_pico_validation_feed(&r, '\r'), READ_PICO_VALIDATION_STATUS);
    EXPECT_EQ(read_pico_validation_feed(&r, '\n'), READ_PICO_VALIDATION_NONE);
}

TEST(ReadPicoValidation, InvalidCommandNeverChangesStateOrCallsIo)
{
    Fake f;
    auto s = begin(f);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_INVALID));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_NONE));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STATUS));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_CLEAR);
    EXPECT_EQ(f.clears + f.confirmations + f.stops, 0);
}

TEST(ReadPicoValidation, PatternsRequireCurrentBootWhiteReference)
{
    Fake f;
    auto s = begin(f);
    s.ops.draw_pattern = draw;
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
    EXPECT_EQ(f.draws, 0);
    EXPECT_EQ(s.drawn_mask, 0u);
}

TEST(ReadPicoValidation, WhiteOnlySessionRejectsPatternsEvenAfterConfirmation)
{
    Fake f;
    auto s = begin(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_REFERENCE_KNOWN);
    EXPECT_EQ(f.draws, 0);
}

TEST(ReadPicoValidation, SuccessfulDrawWaitsForVisualConfirmationAndDoesNotAdvance)
{
    Fake f;
    auto s = ready(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    EXPECT_EQ(f.pattern, READ_PICO_PATTERN_GRAY16);
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_PATTERN);
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_GRAY16);
    EXPECT_EQ(s.drawn_mask, 1u << READ_PICO_PATTERN_GRAY16);
    EXPECT_EQ(s.confirmed_mask, 0u);
    for (auto c : {READ_PICO_VALIDATION_DRAW_GRAY16, READ_PICO_VALIDATION_DRAW_CORNERS,
                   READ_PICO_VALIDATION_CLEAR_WHITE, READ_PICO_VALIDATION_CONFIRM_WHITE})
        EXPECT_FALSE(read_pico_validation_apply(&s, c));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STATUS));
    EXPECT_EQ(f.draws, 1);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_REFERENCE_KNOWN);
    EXPECT_EQ(s.confirmed_mask, s.drawn_mask);
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_COUNT);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    EXPECT_EQ(f.draws, 1);
}

TEST(ReadPicoValidation, AllCommandsMapToOneShotPatternsAndManualAcknowledgements)
{
    Fake f;
    auto s = ready(f);
    for (int p = 0; p < READ_PICO_PATTERN_GRAY16_VENDOR; ++p) {
        auto c = static_cast<read_pico_validation_command_t>(READ_PICO_VALIDATION_DRAW_WHITE + p);
        ASSERT_TRUE(read_pico_validation_apply(&s, c));
        EXPECT_EQ(f.pattern, p);
        EXPECT_EQ(s.confirmed_mask, (1u << p) - 1);
        ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
        EXPECT_FALSE(read_pico_validation_apply(&s, c));
    }
    EXPECT_EQ(s.confirmed_mask, 0x7fu);
    EXPECT_EQ(s.drawn_mask, 0x7fu);
    EXPECT_EQ(f.draws, 7);
    EXPECT_EQ(f.clears, 1);
    EXPECT_EQ(f.confirmations, 1);
}

TEST(ReadPicoValidation, FailedDrawCannotBeConfirmedRetriedOrReplaced)
{
    Fake f;
    f.draw_error = 99;
    auto s = ready(f);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_EQ(s.operation_error, 99);
    EXPECT_EQ(s.drawn_mask, 0u);
    for (auto c : {READ_PICO_VALIDATION_DRAW_GRAY16, READ_PICO_VALIDATION_DRAW_BLACK,
                   READ_PICO_VALIDATION_CONFIRM_PATTERN, READ_PICO_VALIDATION_CONFIRM_WHITE,
                   READ_PICO_VALIDATION_CLEAR_WHITE})
        EXPECT_FALSE(read_pico_validation_apply(&s, c));
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STATUS));
    EXPECT_EQ(f.draws, 1);
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
}

TEST(ReadPicoValidation, VisualRejectionLatchesWithoutIoOrAutomaticClear)
{
    Fake f;
    auto s = ready(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_PARITY));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_EQ(s.confirmed_mask, 0u);
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_PARITY);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_CORNERS));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_EQ(f.draws, 1);
    EXPECT_EQ(f.clears, 1);
    EXPECT_EQ(f.stops, 0);
    EXPECT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
}

TEST(ReadPicoValidation, StopFromPendingPatternIsTerminalAndDoesNotConfirm)
{
    Fake f;
    auto s = ready(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_CORNERS));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_STOPPED);
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_COUNT);
    EXPECT_EQ(s.confirmed_mask, 0u);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_BLACK));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_EQ(f.draws, 1);
    auto fresh = begin(f);
    EXPECT_EQ(fresh.state, READ_PICO_VALIDATION_WAIT_CLEAR);
    EXPECT_EQ(fresh.drawn_mask | fresh.confirmed_mask, 0u);
}

TEST(ReadPicoValidation, PatternParserAcceptsOnlyExactAsciiWhitelist)
{
    const struct {
        const char *name;
        read_pico_validation_command_t command;
    } commands[] = {
        {"gc16-white", READ_PICO_VALIDATION_DRAW_WHITE},
        {"gc16-black", READ_PICO_VALIDATION_DRAW_BLACK},
        {"gc16-midgray", READ_PICO_VALIDATION_DRAW_MIDGRAY},
        {"gc16-gray16", READ_PICO_VALIDATION_DRAW_GRAY16},
        {"gc16-parity", READ_PICO_VALIDATION_DRAW_PARITY},
        {"gc16-corners", READ_PICO_VALIDATION_DRAW_CORNERS},
        {"gc16-gray16-labeled", READ_PICO_VALIDATION_DRAW_GRAY16_LABELED},
        {"gc16-gray16-vendor", READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR},
        {"gc16-gray16-page", READ_PICO_VALIDATION_DRAW_GRAY16_PAGE},
        {"gc16-gray16-labeled-continuous", READ_PICO_VALIDATION_DRAW_GRAY16_LABELED_CONTINUOUS},
        {"pattern-confirm", READ_PICO_VALIDATION_CONFIRM_PATTERN},
        {"pattern-reject", READ_PICO_VALIDATION_REJECT_PATTERN},
    };
    for (const auto &item : commands) {
        EXPECT_EQ(line(std::string(item.name) + "\n"), item.command);
        EXPECT_EQ(line(std::string(item.name) + " x\n"), READ_PICO_VALIDATION_INVALID);
        EXPECT_EQ(line(std::string(item.name) + "\t\n"), READ_PICO_VALIDATION_INVALID);
    }
    EXPECT_EQ(line("gc16-all\n"), READ_PICO_VALIDATION_INVALID);
    EXPECT_EQ(line("GC16-GRAY16\n"), READ_PICO_VALIDATION_INVALID);
    EXPECT_EQ(line(std::string("gc16-gray16\0\n", 13)), READ_PICO_VALIDATION_INVALID);
}

TEST(ReadPicoValidation, ContinuousControlRequiresFreshReferenceAndUsesLabeledVisualGate)
{
    Fake f;
    auto s = begin(f);
    s.ops.draw_white_then_labeled = draw_continuous;
    auto c = READ_PICO_VALIDATION_DRAW_GRAY16_LABELED_CONTINUOUS;
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    ASSERT_TRUE(read_pico_validation_apply(&s, c));
    EXPECT_EQ(f.continuous_draws, 1);
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_WAIT_PATTERN);
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_GRAY16_LABELED);
    EXPECT_EQ(s.drawn_mask, 0x40u);
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
}

TEST(ReadPicoValidation, ContinuousControlFailureLatchesWithoutDrawingMask)
{
    Fake f;
    f.continuous_error = 91;
    auto s = begin(f);
    s.ops.draw_white_then_labeled = draw_continuous;
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    EXPECT_FALSE(
        read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_LABELED_CONTINUOUS));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_EQ(s.operation_error, 91);
    EXPECT_EQ(s.drawn_mask, 0u);
    EXPECT_EQ(f.continuous_draws, 1);
}

TEST(ReadPicoValidation, NumberedGrayPatternRetainsWhiteAndVisualOneShotGates)
{
    Fake f;
    auto s = begin(f);
    s.ops.draw_pattern = draw;
    auto c = READ_PICO_VALIDATION_DRAW_GRAY16_LABELED;
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    ASSERT_TRUE(read_pico_validation_apply(&s, c));
    EXPECT_EQ(s.pending_pattern, READ_PICO_PATTERN_GRAY16_LABELED);
    EXPECT_EQ(s.drawn_mask, 0x40u);
    EXPECT_EQ(s.confirmed_mask, 0u);
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_EQ(f.draws, 1);
}

TEST(ReadPicoValidation, VendorControlRequiresUntouchedManualWhiteAndVisualRejection)
{
    Fake f;
    auto s = begin(f);
    s.ops.draw_pattern = draw;
    auto c = READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR;
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CLEAR_WHITE));
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_WHITE));
    ASSERT_TRUE(read_pico_validation_apply(&s, c));
    EXPECT_EQ(s.drawn_mask, 0x80u);
    EXPECT_EQ(s.confirmed_mask, 0u);
    EXPECT_FALSE(read_pico_validation_apply(&s, c));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_REJECT_PATTERN));
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_FAULT);
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_STOP));
    EXPECT_EQ(f.draws, 1);
}

TEST(ReadPicoValidation, VendorControlCannotResetReferenceAfterAnotherConfirmedPattern)
{
    Fake f;
    auto s = ready(f);
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16));
    ASSERT_TRUE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_CONFIRM_PATTERN));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_VENDOR));
    EXPECT_FALSE(read_pico_validation_apply(&s, READ_PICO_VALIDATION_DRAW_GRAY16_PAGE));
    EXPECT_EQ(f.draws, 1);
    EXPECT_EQ(s.state, READ_PICO_VALIDATION_REFERENCE_KNOWN);
}
