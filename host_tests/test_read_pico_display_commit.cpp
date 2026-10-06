#include <gtest/gtest.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C" {
#include "GUI_BMPfile.h"
#include "GUI_EPDGZfile.h"
#include "GUI_PNGfile.h"
#include "album_manager.h"
#include "config_manager.h"
#include "display_manager.h"
#include "epaper.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "read_pico_epaper.h"
}

namespace
{
bool locked;
int mutex_token, calls, nvs_writes, last_index;
read_pico_refresh_result_t native_result;
std::vector<uint8_t> submitted;
std::string nvs_image;
sd_rotation_mode_t rotation_mode;
int32_t initial_index = -1;
const char *root = "pf_display_checked";
std::string album_directory;
std::string decoded_image;
std::string link_contents()
{
    std::ifstream file(std::string(root) + "/.current.lnk");
    return std::string(std::istreambuf_iterator<char>(file), {});
}
class DisplayCommit : public testing::Test
{
    std::filesystem::path previous_dir, scratch_dir;

   protected:
    void SetUp() override
    {
        previous_dir = std::filesystem::current_path();
        char path[] = "/tmp/read-pico-display-XXXXXX";
        ASSERT_NE(mkdtemp(path), nullptr);
        scratch_dir = path;
        std::filesystem::current_path(scratch_dir);
        std::filesystem::create_directory(root);
        native_result = {};
        native_result.completed = true;
        static bool ready = false;
        if (!ready) {
            ASSERT_EQ(display_manager_init(), ESP_OK);
            ready = true;
        }
        ASSERT_EQ(display_manager_clear(), ESP_OK);
        calls = nvs_writes = 0;
        submitted.clear();
        nvs_image.clear();
        initial_index = last_index = -1;
        rotation_mode = SD_ROTATION_SEQUENTIAL;
        album_directory = root;
        decoded_image.clear();
    }
    void TearDown() override
    {
        EXPECT_FALSE(display_manager_is_busy());
        std::filesystem::current_path(previous_dir);
        std::filesystem::remove_all(scratch_dir);
    }
    void publish(const char *name = "previous.png")
    {
        ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
        display_publish_t pub = {name, nullptr, nullptr};
        ASSERT_EQ(display_manager_end_rgb_stream(true, &pub), ESP_OK);
        ASSERT_EQ(link_contents(), name);
    }
    void fail(read_pico_refresh_error_t stage = READ_PICO_REFRESH_DRAW_FAILED)
    {
        native_result = {};
        native_result.error = stage;
        native_result.operation_error = 7;
    }
};
}  // namespace

extern "C" {
int test_board_display_width = 1216, test_board_display_height = 684;
const char *test_board_display_type = "gc16";
SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return &mutex_token;
}
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t)
{
    if (locked)
        return pdFALSE;
    locked = true;
    return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t)
{
    EXPECT_TRUE(locked);
    locked = false;
    return pdTRUE;
}
read_pico_refresh_result_t read_pico_epaper_display(const uint8_t *frame, size_t length)
{
    EXPECT_TRUE(locked);
    EXPECT_EQ(length, READ_PICO_FRAME_BYTES);
    submitted.assign(frame, frame + length);
    ++calls;
    return native_result;
}
// Legacy void calls must never be used by the Read Pico business path.
void epaper_display(uint8_t *)
{
    ADD_FAILURE();
}
void epaper_clear(uint8_t *, uint8_t)
{
    ADD_FAILURE();
}
uint32_t esp_get_free_heap_size(void)
{
    return 1000000;
}
uint32_t esp_random(void)
{
    return 0;
}
int config_manager_get_display_rotation_deg(void)
{
    return 0;
}
int32_t config_manager_get_last_index(void)
{
    return initial_index;
}
void config_manager_set_last_index(int32_t value)
{
    last_index = initial_index = value;
}
bool config_manager_get_auto_rotate(void)
{
    return true;
}
sd_rotation_mode_t config_manager_get_sd_rotation_mode(void)
{
    return rotation_mode;
}
bool storage_has_persistent_storage(void)
{
    return true;
}
esp_err_t album_manager_get_enabled_albums(char ***albums, int *count)
{
    *albums = static_cast<char **>(malloc(sizeof(char *)));
    (*albums)[0] = strdup("Default");
    *count = 1;
    return ESP_OK;
}
void album_manager_free_album_list(char **albums, int count)
{
    for (int i = 0; i < count; ++i)
        free(albums[i]);
    free(albums);
}
esp_err_t album_manager_get_album_path(const char *, char *path, size_t length)
{
    snprintf(path, length, "%s", album_directory.c_str());
    return ESP_OK;
}
bool album_manager_album_exists(const char *)
{
    return true;
}
esp_err_t album_manager_set_album_enabled(const char *, bool)
{
    return ESP_OK;
}
esp_err_t nvs_open(const char *, int, nvs_handle_t *handle)
{
    *handle = 1;
    return ESP_OK;
}
esp_err_t nvs_get_str(nvs_handle_t, const char *, char *out, size_t *length)
{
    if (*length <= nvs_image.size())
        return ESP_ERR_INVALID_SIZE;
    strcpy(out, nvs_image.c_str());
    *length = nvs_image.size() + 1;
    return ESP_OK;
}
esp_err_t nvs_set_str(nvs_handle_t, const char *, const char *value)
{
    ++nvs_writes;
    nvs_image = value;
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t)
{
    return ESP_OK;
}
void nvs_close(nvs_handle_t) {}
UBYTE GUI_ReadBmp_RGB_Gray16(const char *, UWORD, UWORD)
{
    return 0;
}
UBYTE GUI_ReadBmp_RGB_6Color(const char *, UWORD, UWORD)
{
    return 0;
}
UBYTE GUI_ReadPng_Gray16(const char *path, UWORD, UWORD)
{
    decoded_image = path;
    return 0;
}
UBYTE GUI_ReadPng_RGB_6Color(const char *, UWORD, UWORD)
{
    return 0;
}
int GUI_ReadEPDGZ(const char *)
{
    return 0;
}
}

TEST_F(DisplayCommit, StreamFailurePreservesPreviousImageLinkAndDoesNotSaveSnapshot)
{
    publish();
    fail();
    ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    display_publish_t pub = {"next.png", "pf_display_checked/new.epdgz", "fallback"};
    EXPECT_EQ(display_manager_end_rgb_stream(true, &pub), ESP_FAIL);
    EXPECT_STREQ(display_manager_get_current_image(), "previous.png");
    EXPECT_EQ(link_contents(), "previous.png");
    EXPECT_FALSE(std::filesystem::exists(pub.save_path));
    EXPECT_EQ(nvs_writes, 0);
}
TEST_F(DisplayCommit, CleanupFailureIsNotCommittedEvenAfterDraw)
{
    publish();
    fail(READ_PICO_REFRESH_STOP_FAILED);
    native_result.cleanup_error = 9;
    EXPECT_EQ(display_manager_show_image("next.png"), ESP_FAIL);
    EXPECT_STREQ(display_manager_get_current_image(), "previous.png");
    EXPECT_EQ(link_contents(), "previous.png");
}
TEST_F(DisplayCommit, RgbAndCalibrationFailureReleaseLockAndKeepRecord)
{
    publish();
    fail();
    const uint8_t rgb[] = {0, 0, 0};
    EXPECT_EQ(display_manager_show_rgb_buffer(rgb, 1, 1), ESP_FAIL);
    EXPECT_EQ(display_manager_show_calibration(), ESP_FAIL);
    EXPECT_EQ(link_contents(), "previous.png");
    EXPECT_EQ(nvs_writes, 0);
}
TEST_F(DisplayCommit, BusyReturnsTimeoutWithoutChangingState)
{
    publish();
    fail(READ_PICO_REFRESH_BUSY);
    EXPECT_EQ(display_manager_show_image("next.epdgz"), ESP_ERR_TIMEOUT);
    EXPECT_EQ(link_contents(), "previous.png");
}
TEST_F(DisplayCommit, FailedClearPreservesRecordAndDoesNotWriteNvs)
{
    publish();
    fail();
    EXPECT_EQ(display_manager_clear(), ESP_FAIL);
    EXPECT_EQ(link_contents(), "previous.png");
    EXPECT_EQ(nvs_writes, 0);
}
TEST_F(DisplayCommit, SuccessfulClearUsesGrayWhiteAndOnlyOneCheckedRefresh)
{
    publish();
    calls = 0;
    ASSERT_EQ(display_manager_clear(), ESP_OK);
    EXPECT_EQ(calls, 1);
    ASSERT_EQ(submitted.size(), READ_PICO_FRAME_BYTES);
    for (uint8_t byte : submitted)
        ASSERT_EQ(byte, 0xff);
    EXPECT_STREQ(display_manager_get_current_image(), "");
    EXPECT_FALSE(std::filesystem::exists(std::string(root) + "/.current.lnk"));
    EXPECT_EQ(nvs_writes, 1);
    EXPECT_EQ(nvs_image, "");
}
TEST_F(DisplayCommit, SuccessfulImagePublishesAfterCheckedRefresh)
{
    ASSERT_EQ(display_manager_show_image("next.png"), ESP_OK);
    EXPECT_EQ(link_contents(), "next.png");
    EXPECT_EQ(calls, 1);
}
TEST_F(DisplayCommit, CancelledStreamDoesNotRefreshOrPublish)
{
    publish();
    int before = calls;
    ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    EXPECT_EQ(display_manager_end_rgb_stream(false, nullptr), ESP_OK);
    EXPECT_EQ(calls, before);
    EXPECT_EQ(link_contents(), "previous.png");
}
TEST_F(DisplayCommit, SequentialFailureDoesNotSaveLastImageOrAdvanceIndex)
{
    std::ofstream(std::string(root) + "/photo.png") << "fixture";
    fail();
    EXPECT_EQ(display_manager_rotate_from_storage_checked(), ESP_FAIL);
    EXPECT_EQ(nvs_writes, 0);
    EXPECT_EQ(last_index, -1);
    EXPECT_EQ(calls, 1);
}
TEST_F(DisplayCommit, SequentialWrapFailureDoesNotSaveLastImageOrResetIndex)
{
    std::ofstream(std::string(root) + "/photo.png") << "fixture";
    initial_index = last_index = 9;
    fail();
    EXPECT_EQ(display_manager_rotate_from_storage_checked(), ESP_FAIL);
    EXPECT_EQ(nvs_writes, 0);
    EXPECT_EQ(last_index, 9);
    EXPECT_EQ(calls, 1);
}
TEST_F(DisplayCommit, SequentialWrapKeepsMaximumFittingFirstPathTerminated)
{
    album_directory = std::string(root) + "/" + std::string(236, 'a');
    ASSERT_EQ(album_directory.size(), 255u);
    std::filesystem::create_directories(album_directory);
    std::string image = album_directory + "/" + std::string(251, 'b') + ".png";
    ASSERT_EQ(image.size(), 511u);
    std::ofstream(image) << "fixture";
    initial_index = last_index = 9;
    ASSERT_EQ(display_manager_rotate_from_storage_checked(), ESP_OK);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(last_index, 0);
    EXPECT_EQ(decoded_image, image);
    EXPECT_EQ(nvs_image, image);
    EXPECT_EQ(display_manager_get_current_image(), image);
    EXPECT_EQ(link_contents(), image);
}
TEST_F(DisplayCommit, RandomFailureDoesNotSaveLastImage)
{
    std::ofstream(std::string(root) + "/photo.png") << "fixture";
    rotation_mode = SD_ROTATION_RANDOM;
    fail();
    EXPECT_EQ(display_manager_rotate_from_storage_checked(), ESP_FAIL);
    EXPECT_EQ(nvs_writes, 0);
    EXPECT_EQ(calls, 1);
}

TEST_F(DisplayCommit, OversizedFilePathIsRejectedBeforeDecodeRefreshOrPublish)
{
    publish();
    int before = calls;
    std::string path(DISPLAY_IMAGE_PATH_CAPACITY, 'x');
    EXPECT_EQ(display_manager_show_image(path.c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(calls, before);
    EXPECT_TRUE(decoded_image.empty());
    EXPECT_EQ(link_contents(), "previous.png");
    EXPECT_STREQ(display_manager_get_current_image(), "previous.png");
}

TEST_F(DisplayCommit, EveryOversizedStreamPublicationPathIsRejectedBeforeRefresh)
{
    publish();
    int before = calls;
    std::string oversized(DISPLAY_IMAGE_PATH_CAPACITY, 'x');
    display_publish_t invalid[] = {
        {oversized.c_str(), nullptr, nullptr},
        {"next.png", oversized.c_str(), "fallback.png"},
        {"next.png", "pf_display_checked/new.epdgz", oversized.c_str()},
    };
    for (const auto &pub : invalid) {
        ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
        EXPECT_EQ(display_manager_end_rgb_stream(true, &pub), ESP_ERR_INVALID_SIZE);
        EXPECT_FALSE(display_manager_is_busy());
        EXPECT_EQ(calls, before);
        EXPECT_EQ(link_contents(), "previous.png");
        EXPECT_STREQ(display_manager_get_current_image(), "previous.png");
        EXPECT_FALSE(std::filesystem::exists("pf_display_checked/new.epdgz"));
    }
}

TEST_F(DisplayCommit, MaximumStreamNameAndSnapshotFallbackAreNotTruncated)
{
    std::string name(DISPLAY_IMAGE_PATH_CAPACITY - 1, 'n');
    ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    display_publish_t pub = {name.c_str(), nullptr, nullptr};
    ASSERT_EQ(display_manager_end_rgb_stream(true, &pub), ESP_OK);
    EXPECT_EQ(display_manager_get_current_image(), name);
    EXPECT_EQ(link_contents(), name);
    std::string fallback(DISPLAY_IMAGE_PATH_CAPACITY - 1, 'f');
    pub = {"next.png", "missing/snapshot.epdgz", fallback.c_str()};
    ASSERT_EQ(display_manager_begin_rgb_stream(), ESP_OK);
    EXPECT_EQ(display_manager_end_rgb_stream(true, &pub), ESP_ERR_NOT_FINISHED);
    EXPECT_EQ(display_manager_get_current_image(), fallback);
    EXPECT_EQ(link_contents(), fallback);
}
