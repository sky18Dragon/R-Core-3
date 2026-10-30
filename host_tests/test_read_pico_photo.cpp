#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <string>
#include <vector>
extern "C" {
#include "read_pico_photo_frame.h"
#include "read_pico_photo_session.h"
}

namespace
{
struct Ops {
    int calls[5] = {}, errors[5] = {};
    photo_session_t *session = nullptr;
    bool reenter = false;
};
int call(void *ctx, unsigned index)
{
    auto &o = *static_cast<Ops *>(ctx);
    ++o.calls[index];
    if (o.reenter) {
        EXPECT_FALSE(photo_session_apply(o.session, PHOTO_SHOW));
        EXPECT_FALSE(photo_session_apply(o.session, PHOTO_STOP));
        EXPECT_TRUE(photo_session_apply(o.session, PHOTO_STATUS));
    }
    return o.errors[index];
}
int mount(void *o)
{
    return call(o, 0);
}
int clear(void *o)
{
    return call(o, 1);
}
int confirm(void *o)
{
    return call(o, 2);
}
int show(void *o)
{
    return call(o, 3);
}
int stop(void *o)
{
    return call(o, 4);
}
photo_session_t init(Ops &o)
{
    photo_ops_t ops{&o, mount, clear, confirm, show, stop};
    photo_session_t s;
    EXPECT_TRUE(photo_session_init(&s, &ops));
    return s;
}
void ready(photo_session_t &s)
{
    ASSERT_TRUE(photo_session_apply(&s, PHOTO_MOUNT));
    ASSERT_TRUE(photo_session_apply(&s, PHOTO_CLEAR));
    ASSERT_TRUE(photo_session_apply(&s, PHOTO_WHITE_CONFIRM));
}
photo_command_t line(const std::string &str)
{
    photo_reader_t r{};
    for (unsigned char b : str)
        EXPECT_EQ(photo_session_feed(&r, b), PHOTO_NONE);
    return photo_session_feed(&r, '\n');
}
}  // namespace

TEST(PhotoSession, InitDoesNoIoAndRejectsMissingCallbacks)
{
    Ops o;
    auto s = init(o);
    EXPECT_EQ(s.state, PHOTO_WAIT_MOUNT);
    for (int n : o.calls)
        EXPECT_EQ(n, 0);
    photo_ops_t ops{&o, mount, clear, confirm, show, nullptr};
    EXPECT_FALSE(photo_session_init(&s, &ops));
    EXPECT_FALSE(photo_session_init(nullptr, &ops));
    EXPECT_FALSE(photo_session_init(&s, nullptr));
    photo_session_t empty{};
    EXPECT_FALSE(photo_session_apply(&empty, PHOTO_STOP));
}
TEST(PhotoSession, NoClearWithoutMountOrPhotoWithoutHumanWhite)
{
    Ops o;
    auto s = init(o);
    for (auto c : {PHOTO_CLEAR, PHOTO_WHITE_CONFIRM, PHOTO_SHOW, PHOTO_CONFIRM, PHOTO_REJECT})
        EXPECT_FALSE(photo_session_apply(&s, c));
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_MOUNT));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_SHOW));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_MOUNT));
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_CLEAR));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_CLEAR));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_SHOW));
    EXPECT_EQ(o.calls[3], 0);
}
TEST(PhotoSession, OnePhotoRequiresSeparateHumanAcceptance)
{
    Ops o;
    auto s = init(o);
    ready(s);
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_SHOW));
    EXPECT_EQ(s.state, PHOTO_WAIT_VISUAL);
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_SHOW));
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_CONFIRM));
    EXPECT_EQ(s.state, PHOTO_VERIFIED);
    for (auto c : {PHOTO_MOUNT, PHOTO_CLEAR, PHOTO_SHOW, PHOTO_CONFIRM, PHOTO_REJECT})
        EXPECT_FALSE(photo_session_apply(&s, c));
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
    EXPECT_EQ(s.state, PHOTO_STOPPED);
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
    for (int n : o.calls)
        EXPECT_EQ(n, 1);
}
TEST(PhotoSession, AnyOperationFailureLatchesAndCannotBeRetried)
{
    for (unsigned fail = 0; fail < 4; ++fail) {
        Ops o;
        auto s = init(o);
        o.errors[fail] = 90 + fail;
        const photo_command_t commands[] = {PHOTO_MOUNT, PHOTO_CLEAR, PHOTO_WHITE_CONFIRM,
                                            PHOTO_SHOW};
        for (unsigned i = 0; i <= fail; ++i)
            EXPECT_EQ(photo_session_apply(&s, commands[i]), i != fail);
        EXPECT_EQ(s.state, PHOTO_FAULT);
        EXPECT_EQ(s.operation_error, 90 + fail);
        for (auto c : commands)
            EXPECT_FALSE(photo_session_apply(&s, c));
        EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
        EXPECT_EQ(s.operation_error, 90 + fail);
        EXPECT_EQ(o.calls[fail], 1);
    }
}
TEST(PhotoSession, RejectionOfWhiteOrPhotoIsTerminal)
{
    for (bool photo : {false, true}) {
        Ops o;
        auto s = init(o);
        ASSERT_TRUE(photo_session_apply(&s, PHOTO_MOUNT));
        ASSERT_TRUE(photo_session_apply(&s, PHOTO_CLEAR));
        if (photo) {
            ASSERT_TRUE(photo_session_apply(&s, PHOTO_WHITE_CONFIRM));
            ASSERT_TRUE(photo_session_apply(&s, PHOTO_SHOW));
        }
        EXPECT_TRUE(photo_session_apply(&s, PHOTO_REJECT));
        EXPECT_EQ(s.state, PHOTO_FAULT);
        EXPECT_FALSE(photo_session_apply(&s, PHOTO_SHOW));
        EXPECT_FALSE(photo_session_apply(&s, PHOTO_WHITE_CONFIRM));
        EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
        EXPECT_EQ(s.operation_error, -1);
    }
}
TEST(PhotoSession, CleanupFailurePreservesPrimaryAndDoesNotRetry)
{
    Ops o;
    auto s = init(o);
    o.errors[0] = 8;
    o.errors[4] = 9;
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_MOUNT));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_STOP));
    EXPECT_EQ(s.operation_error, 8);
    EXPECT_EQ(s.cleanup_error, 9);
    EXPECT_EQ(s.state, PHOTO_FAULT);
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_STATUS));
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_STOP));
    EXPECT_EQ(o.calls[4], 1);
}
TEST(PhotoSession, StopBeforeAnyActionDoesNotStartAnything)
{
    Ops o;
    auto s = init(o);
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
    for (unsigned i = 0; i < 4; ++i)
        EXPECT_EQ(o.calls[i], 0);
    EXPECT_FALSE(photo_session_apply(&s, PHOTO_MOUNT));
}
TEST(PhotoSession, ReentrantCallsCannotShowOrRepeatOperations)
{
    Ops o;
    auto s = init(o);
    o.session = &s;
    o.reenter = true;
    ready(s);
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_SHOW));
    EXPECT_TRUE(photo_session_apply(&s, PHOTO_STOP));
    EXPECT_EQ(o.calls[3], 1);
}
TEST(PhotoSession, ExactParserWhitelistAndCrLf)
{
    const std::pair<const char *, photo_command_t> commands[] = {
        {"status", PHOTO_STATUS},       {"mount", PHOTO_MOUNT},
        {"clear-white", PHOTO_CLEAR},   {"white-confirm", PHOTO_WHITE_CONFIRM},
        {"photo-show", PHOTO_SHOW},     {"photo-confirm", PHOTO_CONFIRM},
        {"photo-reject", PHOTO_REJECT}, {"stop", PHOTO_STOP}};
    for (auto p : commands)
        EXPECT_EQ(line(p.first), p.second);
    photo_reader_t r{};
    for (char c : std::string("status"))
        EXPECT_EQ(photo_session_feed(&r, c), PHOTO_NONE);
    EXPECT_EQ(photo_session_feed(&r, '\r'), PHOTO_STATUS);
    EXPECT_EQ(photo_session_feed(&r, '\n'), PHOTO_NONE);
}
TEST(PhotoSession, ParserRejectsWholeOverflowControlAndPrefixLines)
{
    for (auto str :
         {" photo-show", "photo-show x", "PHOTO-SHOW", "gc16-white", "file-test", "photo-show\t"})
        EXPECT_EQ(line(str), PHOTO_INVALID);
    EXPECT_EQ(line(std::string("photo-show\0status", 17)), PHOTO_INVALID);
    EXPECT_EQ(line(std::string(100, 'x') + "stop"), PHOTO_INVALID);
    photo_reader_t r{};
    for (char c : std::string(100, 'x'))
        photo_session_feed(&r, c);
    EXPECT_EQ(photo_session_feed(&r, '\n'), PHOTO_INVALID);
    for (char c : std::string("stop"))
        photo_session_feed(&r, c);
    EXPECT_EQ(photo_session_feed(&r, '\n'), PHOTO_STOP);
    EXPECT_EQ(photo_session_feed(nullptr, 0), PHOTO_INVALID);
}

TEST(PhotoFrame, RowsPackHighNibbleFirstWithFullCoverage)
{
    std::vector<uint8_t> bytes(READ_PICO_FRAME_BYTES + 2, 0x31), row(READ_PICO_PANEL_WIDTH * 3);
    photo_frame_t f{};
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data() + 1, READ_PICO_FRAME_BYTES, false));
    for (unsigned x = 0; x < READ_PICO_PANEL_WIDTH; ++x)
        memset(row.data() + x * 3, (x % 16) * 17, 3);
    for (unsigned y = 0; y < READ_PICO_PANEL_HEIGHT; ++y) {
        EXPECT_FALSE(photo_frame_complete(&f));
        ASSERT_TRUE(photo_frame_push(&f, false, y, row.data(), READ_PICO_PANEL_WIDTH));
    }
    EXPECT_TRUE(photo_frame_complete(&f));
    for (size_t i = 0; i < READ_PICO_FRAME_BYTES; ++i)
        EXPECT_EQ(bytes[i + 1], ((2 * i % 16) << 4) | ((2 * i + 1) % 16));
    EXPECT_EQ(bytes.front(), 0x31);
    EXPECT_EQ(bytes.back(), 0x31);
}
TEST(PhotoFrame, PortraitColumnsHaveExpectedNativeOrder)
{
    std::vector<uint8_t> bytes(READ_PICO_FRAME_BYTES), col(READ_PICO_PANEL_HEIGHT * 3);
    photo_frame_t f{};
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
    for (int x = READ_PICO_PANEL_WIDTH - 1; x >= 0; --x) {
        memset(col.data(), (x % 16) * 17, col.size());
        ASSERT_TRUE(photo_frame_push(&f, true, x, col.data(), READ_PICO_PANEL_HEIGHT));
    }
    EXPECT_TRUE(photo_frame_complete(&f));
    for (size_t i = 0; i < bytes.size(); ++i)
        EXPECT_EQ(bytes[i], ((2 * i % 16) << 4) | ((2 * i + 1) % 16));
}
TEST(PhotoFrame, BoardRotationMatchesVendorInvertedPortraitMapping)
{
    std::vector<uint8_t> bytes(READ_PICO_FRAME_BYTES), col(READ_PICO_PANEL_HEIGHT * 3);
    photo_frame_t f{};
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), true));
    for (int logical_y = 0; logical_y < READ_PICO_PANEL_WIDTH; ++logical_y) {
        int native_column = READ_PICO_PANEL_WIDTH - 1 - logical_y;
        for (int logical_x = 0; logical_x < READ_PICO_PANEL_HEIGHT; ++logical_x) {
            uint8_t gray = 15;
            if (logical_x == 0 && logical_y == 0)
                gray = 1;
            else if (logical_x == READ_PICO_PANEL_HEIGHT - 1 && logical_y == 0)
                gray = 2;
            else if (logical_x == 0 && logical_y == READ_PICO_PANEL_WIDTH - 1)
                gray = 3;
            else if (logical_x == READ_PICO_PANEL_HEIGHT - 1 &&
                     logical_y == READ_PICO_PANEL_WIDTH - 1)
                gray = 4;
            memset(col.data() + logical_x * 3, gray * 17, 3);
        }
        ASSERT_TRUE(photo_frame_push(&f, true, native_column, col.data(),
                                     READ_PICO_PANEL_HEIGHT));
    }
    ASSERT_TRUE(photo_frame_complete(&f));
    auto pixel = [&](size_t x, size_t y) {
        uint8_t packed = bytes[(y * READ_PICO_PANEL_WIDTH + x) / 2];
        return (uint8_t) ((x & 1) ? packed & 15 : packed >> 4);
    };
    // EPD_ROT_INVERTED_PORTRAIT: logical (x,y) -> native (y, height-1-x).
    EXPECT_EQ(pixel(0, READ_PICO_PANEL_HEIGHT - 1), 1);
    EXPECT_EQ(pixel(0, 0), 2);
    EXPECT_EQ(pixel(READ_PICO_PANEL_WIDTH - 1, READ_PICO_PANEL_HEIGHT - 1), 3);
    EXPECT_EQ(pixel(READ_PICO_PANEL_WIDTH - 1, 0), 4);
}
TEST(PhotoFrame, InvalidPaletteLineDoesNotMutateAndLatches)
{
    for (unsigned bad : {1u, 2u}) {
        std::vector<uint8_t> bytes(READ_PICO_FRAME_BYTES), row(READ_PICO_PANEL_WIDTH * 3, 0);
        photo_frame_t f{};
        ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
        row[row.size() - bad] = bad;
        EXPECT_FALSE(photo_frame_push(&f, false, 0, row.data(), READ_PICO_PANEL_WIDTH));
        EXPECT_TRUE(f.failed);
        EXPECT_FALSE(photo_frame_complete(&f));
        for (auto b : bytes)
            EXPECT_EQ(b, 255);
        memset(row.data(), 0, row.size());
        EXPECT_FALSE(photo_frame_push(&f, false, 0, row.data(), READ_PICO_PANEL_WIDTH));
    }
}
TEST(PhotoFrame, RejectsDuplicateMissingMixedAndWrongGeometry)
{
    std::vector<uint8_t> bytes(READ_PICO_FRAME_BYTES), rgb(READ_PICO_PANEL_WIDTH * 3, 0);
    photo_frame_t f{};
    for (int index : {-1, 1, (int) READ_PICO_PANEL_HEIGHT}) {
        ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
        EXPECT_FALSE(photo_frame_push(&f, false, index, rgb.data(), READ_PICO_PANEL_WIDTH));
    }
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
    ASSERT_TRUE(photo_frame_push(&f, false, 0, rgb.data(), READ_PICO_PANEL_WIDTH));
    EXPECT_FALSE(photo_frame_push(&f, false, 0, rgb.data(), READ_PICO_PANEL_WIDTH));
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
    ASSERT_TRUE(photo_frame_push(&f, false, 0, rgb.data(), READ_PICO_PANEL_WIDTH));
    EXPECT_FALSE(photo_frame_push(&f, true, 1214, rgb.data(), READ_PICO_PANEL_HEIGHT));
    ASSERT_TRUE(photo_frame_begin(&f, bytes.data(), bytes.size(), false));
    EXPECT_FALSE(photo_frame_push(&f, false, 0, rgb.data(), 10));
    EXPECT_FALSE(photo_frame_begin(&f, bytes.data(), bytes.size() - 1, false));
    EXPECT_FALSE(photo_frame_begin(nullptr, bytes.data(), bytes.size(), false));
    EXPECT_FALSE(photo_frame_complete(nullptr));
}
