#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
extern "C" {
#include "image_processor.h"
#include "read_pico_photo_bridge.h"
#include "read_pico_photo_frame.h"
extern int test_board_display_width, test_board_display_height;
extern const char *test_board_display_type;
extern bool test_jpeg_media_present;
extern size_t test_jpeg_media_calls, test_jpeg_media_fail_at;
extern size_t test_heap_max_request, test_heap_request_limit;
extern unsigned test_palette_load_calls;
}

class PhotoBridge : public ::testing::Test
{
   protected:
    void SetUp() override
    {
        read_pico_photo_bridge_release();
        test_board_display_width = 1216;
        test_board_display_height = 684;
        test_board_display_type = "gc16";
        test_jpeg_media_present = true;
        test_jpeg_media_calls = 0;
        test_jpeg_media_fail_at = SIZE_MAX;
        test_heap_request_limit = SIZE_MAX;
        ASSERT_EQ(image_processor_init_defaults(), ESP_OK);
    }
    void TearDown() override
    {
        read_pico_photo_bridge_release();
    }
};
TEST_F(PhotoBridge, DefaultsInitDoesNotLoadNvsPalette)
{
    unsigned before = test_palette_load_calls;
    EXPECT_EQ(image_processor_init_defaults(), ESP_OK);
    EXPECT_EQ(test_palette_load_calls, before);
}
TEST_F(PhotoBridge, AbortAndMissingLinesDoNotExposeFrame)
{
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
    EXPECT_EQ(display_manager_end_rgb_stream(true, nullptr), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    EXPECT_EQ(display_manager_end_rgb_stream(false, nullptr), ESP_OK);
    EXPECT_EQ(display_manager_end_rgb_stream(false, nullptr), ESP_ERR_INVALID_STATE);
}
TEST_F(PhotoBridge, RejectsPublishingAndNonPalettePixels)
{
    std::vector<uint8_t> row(1216 * 3, 255);
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    row.back() = 17;
    EXPECT_NE(display_manager_push_rgb_row(0, row.data(), 1216), ESP_OK);
    EXPECT_NE(display_manager_end_rgb_stream(true, nullptr), ESP_OK);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
    std::fill(row.begin(), row.end(), 255);
    ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    for (int y = 0; y < 684; ++y)
        ASSERT_EQ(display_manager_push_rgb_row(y, row.data(), 1216), ESP_OK);
    display_publish_t pub{};
    EXPECT_NE(display_manager_end_rgb_stream(true, &pub), ESP_OK);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
}
TEST_F(PhotoBridge, AllocationFailureDoesNotAcquireSink)
{
    test_heap_request_limit = 10;
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_ERR_NO_MEM);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
    test_heap_request_limit = SIZE_MAX;
    EXPECT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    EXPECT_EQ(display_manager_end_rgb_stream(false, nullptr), ESP_OK);
}
TEST_F(PhotoBridge, RealFilePipelineProducesFullNativeFourBppWithoutHardware)
{
    ASSERT_EQ(image_processor_process_jpeg_file_to_display(JPEG_FIXTURE_DIR "/landscape.jpg",
                                                           DITHER_FLOYD_STEINBERG, nullptr),
              ESP_OK);
    const uint8_t *f = read_pico_photo_bridge_frame();
    ASSERT_NE(f, nullptr);
    bool dark = false, light = false;
    for (size_t i = 0; i < READ_PICO_FRAME_BYTES; ++i) {
        dark |= (f[i] & 15) < 4 || (f[i] >> 4) < 4;
        light |= (f[i] & 15) > 12 || (f[i] >> 4) > 12;
    }
    EXPECT_TRUE(dark);
    EXPECT_TRUE(light);
    EXPECT_NE(display_manager_begin_rgb_stream(), ESP_OK);
    read_pico_photo_bridge_release();
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
}
TEST_F(PhotoBridge, MissingMediaAndInvalidAlgorithmDoNotProduceFrame)
{
    test_jpeg_media_present = false;
    EXPECT_NE(image_processor_process_jpeg_file_to_display(JPEG_FIXTURE_DIR "/landscape.jpg",
                                                           DITHER_FLOYD_STEINBERG, nullptr),
              ESP_OK);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
    test_jpeg_media_present = true;
    EXPECT_EQ(image_processor_process_jpeg_file_to_display(JPEG_FIXTURE_DIR "/landscape.jpg",
                                                           (dither_algorithm_t) 99, nullptr),
              ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_photo_bridge_frame(), nullptr);
}
