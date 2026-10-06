#include <gtest/gtest.h>

extern "C" {
#include "nvs_bootstrap.h"
#include "nvs_flash.h"
}

static int init_calls;
static esp_err_t init_result;
extern "C" esp_err_t nvs_flash_init(void)
{
    ++init_calls;
    return init_result;
}
// Deliberately no erase mock: an erase dependency must fail this target's link.

TEST(ReadPicoNvsBootstrap, EveryErrorIsReturnedWithoutRecoveryOrRetry)
{
    for (esp_err_t err : {ESP_OK, ESP_ERR_NVS_NO_FREE_PAGES, ESP_ERR_NVS_NEW_VERSION_FOUND,
                          ESP_ERR_NOT_FOUND, ESP_FAIL}) {
        init_calls = 0;
        init_result = err;
        EXPECT_EQ(photoframe_nvs_init(false), err);
        EXPECT_EQ(init_calls, 1);
    }
}
TEST(ReadPicoNvsBootstrap, AccidentalLegacyOptInCannotEnableErase)
{
    init_calls = 0;
    init_result = ESP_ERR_NVS_NO_FREE_PAGES;
    EXPECT_EQ(photoframe_nvs_init(true), ESP_ERR_NVS_NO_FREE_PAGES);
    EXPECT_EQ(init_calls, 1);
}
