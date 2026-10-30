#include <gtest/gtest.h>

extern "C" {
#include "nvs_bootstrap.h"
#include "nvs_flash.h"
}

namespace
{
int init_calls, erase_calls;
esp_err_t first_init, next_init, erase_result;
}  // namespace

extern "C" esp_err_t nvs_flash_init(void)
{
    return ++init_calls == 1 ? first_init : next_init;
}
extern "C" esp_err_t nvs_flash_erase(void)
{
    ++erase_calls;
    return erase_result;
}

class NvsBootstrap : public testing::Test
{
   protected:
    void SetUp() override
    {
        init_calls = erase_calls = 0;
        first_init = next_init = erase_result = ESP_OK;
    }
};

TEST_F(NvsBootstrap, PreservePolicyNeverErasesOrRetriesAnyInitResult)
{
    for (esp_err_t err : {ESP_OK, ESP_ERR_NVS_NO_FREE_PAGES, ESP_ERR_NVS_NEW_VERSION_FOUND,
                          ESP_ERR_NOT_FOUND, ESP_FAIL}) {
        init_calls = erase_calls = 0;
        first_init = err;
        EXPECT_EQ(photoframe_nvs_init(false), err);
        EXPECT_EQ(init_calls, 1);
        EXPECT_EQ(erase_calls, 0);
    }
}
TEST_F(NvsBootstrap, LegacyRecoveryOnlyErasesForKnownIncompatibleStates)
{
    for (esp_err_t err : {ESP_ERR_NVS_NO_FREE_PAGES, ESP_ERR_NVS_NEW_VERSION_FOUND}) {
        init_calls = erase_calls = 0;
        first_init = err;
        EXPECT_EQ(photoframe_nvs_init(true), ESP_OK);
        EXPECT_EQ(init_calls, 2);
        EXPECT_EQ(erase_calls, 1);
    }
}
TEST_F(NvsBootstrap, LegacySuccessOrOtherFailuresDoNotErase)
{
    for (esp_err_t err : {ESP_OK, ESP_FAIL, ESP_ERR_NOT_FOUND}) {
        init_calls = erase_calls = 0;
        first_init = err;
        EXPECT_EQ(photoframe_nvs_init(true), err);
        EXPECT_EQ(init_calls, 1);
        EXPECT_EQ(erase_calls, 0);
    }
}
TEST_F(NvsBootstrap, EraseFailureStopsWithoutRetry)
{
    first_init = ESP_ERR_NVS_NO_FREE_PAGES;
    erase_result = ESP_ERR_TIMEOUT;
    EXPECT_EQ(photoframe_nvs_init(true), ESP_ERR_TIMEOUT);
    EXPECT_EQ(init_calls, 1);
    EXPECT_EQ(erase_calls, 1);
}
TEST_F(NvsBootstrap, RetryErrorIsReturnedWithoutSecondErase)
{
    first_init = ESP_ERR_NVS_NEW_VERSION_FOUND;
    next_init = ESP_ERR_NVS_NO_FREE_PAGES;
    EXPECT_EQ(photoframe_nvs_init(true), ESP_ERR_NVS_NO_FREE_PAGES);
    EXPECT_EQ(init_calls, 2);
    EXPECT_EQ(erase_calls, 1);
}
