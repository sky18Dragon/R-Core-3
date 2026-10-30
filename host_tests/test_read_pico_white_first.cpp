#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "read_pico_white_first.h"
}

namespace {
constexpr size_t kBytes = 64;
std::vector<std::string> calls;
std::vector<std::vector<uint8_t>> drawn;
EpdDrawError full_result = EPD_DRAW_SUCCESS, white_result = EPD_DRAW_SUCCESS;
bool healthy_answer = true;
int healthy_calls;
bool healthy() { ++healthy_calls; return healthy_answer; }
}

extern "C" {
EpdDrawError epd_hl_update_screen_full(EpdiyHighlevelState *hl, int mode, int temperature)
{
    EXPECT_EQ(mode, MODE_GC16);
    EXPECT_EQ(temperature, 31);
    calls.push_back("full");
    drawn.emplace_back(hl->front_fb, hl->front_fb + kBytes);
    memcpy(hl->back_fb, hl->front_fb, kBytes); // vendor HL updates back_fb even on failure
    return full_result;
}
EpdDrawError epd_hl_update_screen_from_white(EpdiyHighlevelState *hl, int mode, int temperature)
{
    EXPECT_EQ(mode, MODE_GC16);
    EXPECT_EQ(temperature, 31);
    calls.push_back("from_white");
    drawn.emplace_back(hl->front_fb, hl->front_fb + kBytes);
    memset(hl->back_fb, 0xff, kBytes);
    memcpy(hl->back_fb, hl->front_fb, kBytes);
    return white_result;
}
}

class ReadPicoWhiteFirst : public testing::Test {
protected:
    uint8_t front[kBytes], back[kBytes], saved[kBytes];
    EpdiyHighlevelState hl{};
    std::vector<uint8_t> target;
    void SetUp() override
    {
        calls.clear();
        drawn.clear();
        full_result = white_result = EPD_DRAW_SUCCESS;
        healthy_answer = true;
        healthy_calls = 0;
        for (size_t i = 0; i < kBytes; ++i) {
            front[i] = uint8_t(0x12 + i);  // new photo
            back[i] = uint8_t(0x80 ^ i);   // previous photo on the panel
        }
        target.assign(front, front + kBytes);
        hl.front_fb = front;
        hl.back_fb = back;
    }
};

TEST_F(ReadPicoWhiteFirst, PhotoChangeFlashesWhiteThenDrawsTargetFromWhite)
{
    EXPECT_EQ(read_pico_white_first_gc16(&hl, saved, kBytes, false, 31, healthy), EPD_DRAW_SUCCESS);
    ASSERT_EQ(calls, (std::vector<std::string>{"full", "from_white"}));
    EXPECT_EQ(drawn[0], std::vector<uint8_t>(kBytes, 0xff)); // first pass: all white
    EXPECT_EQ(drawn[1], target);                             // second pass: the photo
    EXPECT_EQ(std::vector<uint8_t>(front, front + kBytes), target);
    EXPECT_EQ(healthy_calls, 1);
}

TEST_F(ReadPicoWhiteFirst, AlreadyWhitePanelSkipsTheFlash)
{
    EXPECT_EQ(read_pico_white_first_gc16(&hl, saved, kBytes, true, 31, healthy), EPD_DRAW_SUCCESS);
    ASSERT_EQ(calls, (std::vector<std::string>{"from_white"}));
    EXPECT_EQ(drawn[0], target);
    EXPECT_EQ(healthy_calls, 0);
}

TEST_F(ReadPicoWhiteFirst, FlashFailureStopsAndRestoresTargetBuffer)
{
    full_result = MOCK_DRAW_FAILURE;
    EXPECT_EQ(read_pico_white_first_gc16(&hl, saved, kBytes, false, 31, healthy), MOCK_DRAW_FAILURE);
    ASSERT_EQ(calls, (std::vector<std::string>{"full"}));
    EXPECT_EQ(std::vector<uint8_t>(front, front + kBytes), target);
    EXPECT_EQ(healthy_calls, 0);
}

TEST_F(ReadPicoWhiteFirst, UnhealthyBusAfterFlashNeverStartsSecondTransaction)
{
    healthy_answer = false;
    EXPECT_EQ(read_pico_white_first_gc16(&hl, saved, kBytes, false, 31, healthy), -1);
    ASSERT_EQ(calls, (std::vector<std::string>{"full"}));
    EXPECT_EQ(std::vector<uint8_t>(front, front + kBytes), target);
}

TEST_F(ReadPicoWhiteFirst, FromWhiteFailureIsReturned)
{
    white_result = MOCK_DRAW_FAILURE;
    EXPECT_EQ(read_pico_white_first_gc16(&hl, saved, kBytes, false, 31, healthy), MOCK_DRAW_FAILURE);
    ASSERT_EQ(calls, (std::vector<std::string>{"full", "from_white"}));
}
