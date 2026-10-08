#include <gtest/gtest.h>
#include <cstring>
extern "C" {
#include "ota_manager.h"
#include "esp_app_desc.h"
#include "freertos/semphr.h"
}
namespace { int token; bool locked; bool allocation_failed; }
extern "C" {
const esp_app_desc_t *esp_app_get_description(void) {
    static esp_app_desc_t app = {"test-build"}; return &app;
}
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return allocation_failed ? nullptr : &token; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) {
    if (locked) return pdFALSE;
    locked = true; return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t) { locked = false; return pdTRUE; }
}
TEST(ReadPicoOtaGate, InitializationOnlyReportsBuildAndUnacceptedStatus) {
    allocation_failed = locked = false;
    ASSERT_EQ(ota_manager_init(), ESP_OK);
    ota_status_t status{}; ota_get_status(&status);
    EXPECT_EQ(status.state, OTA_STATE_ERROR); EXPECT_STREQ(status.current_version, "test-build");
    EXPECT_STREQ(status.latest_version, ""); EXPECT_EQ(status.progress_percent, 0);
    EXPECT_NE(strstr(status.error_message, "migration"), nullptr);
    EXPECT_STREQ(ota_get_current_version(), "test-build");
    EXPECT_FALSE(ota_should_check_daily()); ota_update_last_check_time(); ota_get_status(nullptr);
    // This fixture has NO NVS/network/partition/periodic/task/restart symbols;
    // accidental linkage of any such operation makes the test binary fail.
}
TEST(ReadPicoOtaGate, ExplicitAndAutomaticChecksAndUpdatesCannotLaunchWork) {
    allocation_failed = locked = false;
    ASSERT_EQ(ota_manager_init(), ESP_OK);
    bool available = true;
    EXPECT_EQ(ota_check_for_update(&available, 30), ESP_ERR_NOT_SUPPORTED);
    EXPECT_FALSE(available);
    EXPECT_EQ(ota_check_for_update(nullptr, 0), ESP_ERR_NOT_SUPPORTED);
    EXPECT_EQ(ota_start_update(), ESP_ERR_NOT_SUPPORTED);
    EXPECT_FALSE(ota_should_check_daily());
}
TEST(ReadPicoOtaGate, MutexFailureDoesNotAuthorizeUpdateOrCheck) {
    allocation_failed = true;
    EXPECT_EQ(ota_manager_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(ota_start_update(), ESP_ERR_NOT_SUPPORTED);
    bool available = true;
    EXPECT_EQ(ota_check_for_update(&available, 0), ESP_ERR_NOT_SUPPORTED); EXPECT_FALSE(available);
    allocation_failed = false;
}
