#include <gtest/gtest.h>
#include <vector>
extern "C" {
#include "read_pico_http_service.h"
}
namespace {
const auto frame = READ_PICO_HTTP_FRAME;
const auto provision = READ_PICO_HTTP_PROVISION;
httpd_handle_t handle = reinterpret_cast<void *>(0x1234);
esp_err_t start_error, stop_error, routes_error;
bool null_success, unexpected_handle, nested;
unsigned starts, stops, registrations;
std::vector<httpd_handle_t> closed;
httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
esp_err_t routes(httpd_handle_t received) {
    EXPECT_EQ(received, handle); ++registrations;
    EXPECT_FALSE(read_pico_http_active(frame));
    EXPECT_FALSE(read_pico_http_accepts(frame, received));
    if (nested) {
        EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_stop(frame), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_ERR_INVALID_STATE);
    }
    return routes_error;
}
class ReadPicoHttpService : public testing::Test {
    void SetUp() override {
        start_error = stop_error = routes_error = ESP_OK;
        null_success = unexpected_handle = nested = false;
        starts = stops = registrations = 0; closed.clear();
        configuration = HTTPD_DEFAULT_CONFIG();
        // Each CTest case is a fresh process, including irreversible SDK faults.
    }
};
}
extern "C" {
esp_err_t httpd_start(httpd_handle_t *output, const httpd_config_t *config) {
    EXPECT_EQ(*output, nullptr); EXPECT_EQ(config->server_port, 80u); ++starts;
    EXPECT_FALSE(read_pico_http_active(frame));
    if ((start_error == ESP_OK && !null_success) || unexpected_handle) *output = handle;
    if (nested) EXPECT_EQ(read_pico_http_stop(frame), ESP_ERR_INVALID_STATE);
    return start_error;
}
esp_err_t httpd_stop(httpd_handle_t received) {
    EXPECT_EQ(received, handle); ++stops; closed.push_back(received);
    EXPECT_FALSE(read_pico_http_active(frame));
    EXPECT_FALSE(read_pico_http_accepts(frame, received));
    if (nested) {
        EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_http_stop(frame), ESP_ERR_INVALID_STATE);
    }
    return stop_error;
}
}
TEST_F(ReadPicoHttpService, ClaimReservesThePortBeforeAnySdkAndRequiresExplicitRelease) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_claim(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(starts, 0u); EXPECT_EQ(stops, 0u);
    EXPECT_EQ(read_pico_http_stop(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_stop(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_claim(provision), ESP_OK);
}
TEST_F(ReadPicoHttpService, StartsOnlyOnceAndOnlyFullyRegisteredMarkedServiceAcceptsRequests) {
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    EXPECT_TRUE(read_pico_http_active(frame)); EXPECT_FALSE(read_pico_http_ready(frame));
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(starts, 1u); EXPECT_EQ(registrations, 1u);
    EXPECT_EQ(read_pico_http_mark_ready(provision), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_http_mark_ready(frame), ESP_OK);
    EXPECT_TRUE(read_pico_http_accepts(frame, handle));
    EXPECT_FALSE(read_pico_http_accepts(provision, handle));
    EXPECT_FALSE(read_pico_http_accepts(frame, reinterpret_cast<void *>(0x5678)));
    EXPECT_FALSE(read_pico_http_accepts(frame, nullptr));
    EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(read_pico_http_accepts(frame, handle));
    ASSERT_EQ(read_pico_http_stop(frame), ESP_OK); EXPECT_EQ(stops, 1u);
    EXPECT_FALSE(read_pico_http_accepts(frame, handle));
    EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
}
TEST_F(ReadPicoHttpService, InvalidArgumentsLeaveOwnershipAndSdkUnchanged) {
    const auto invalid = static_cast<read_pico_http_role_t>(99);
    EXPECT_EQ(read_pico_http_claim(invalid), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_claim(READ_PICO_HTTP_NONE), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_stop(invalid), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_release(invalid), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_mark_ready(invalid), ESP_ERR_INVALID_ARG);
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_start(frame, nullptr, routes), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_start(frame, &configuration, nullptr), ESP_ERR_INVALID_ARG);
    configuration.server_port = 81;
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(starts, 0u); EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
}
TEST_F(ReadPicoHttpService, ForeignServiceCannotStartStopReleaseOrWithdrawReadyFrame) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_start(provision, &configuration, routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(starts, 0u);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    ASSERT_EQ(read_pico_http_mark_ready(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_stop(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_mark_ready(provision), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(read_pico_http_accepts(frame, handle)); EXPECT_EQ(stops, 0u);
    ASSERT_EQ(read_pico_http_stop(frame), ESP_OK); EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
}
TEST_F(ReadPicoHttpService, ReentrantStartRoutesAndStopCannotReplaceOrReleaseSdkObject) {
    nested = true;
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_stop(frame), ESP_OK);
    EXPECT_EQ(starts, 1u); EXPECT_EQ(stops, 1u);
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
}
TEST_F(ReadPicoHttpService, StartupFailureDoesNotGuessAHandleAndCallerMayExplicitlyRelease) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK); start_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_NO_MEM);
    EXPECT_EQ(registrations, 0u); EXPECT_EQ(read_pico_http_stop(frame), ESP_OK); EXPECT_EQ(stops, 0u);
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
    start_error = ESP_OK; ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
}
TEST_F(ReadPicoHttpService, RoutesFailureKeepsExactHandleAndDisablesRequestsBeforeCallerCleanup) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK); routes_error = ESP_ERR_INVALID_ARG;
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(read_pico_http_accepts(frame, handle));
    EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_stop(frame), ESP_OK); EXPECT_EQ(closed, std::vector<httpd_handle_t>{handle});
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
}
TEST_F(ReadPicoHttpService, StopFailureRetainsRoleAndHandleForExplicitRetryNotNewStart) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    ASSERT_EQ(read_pico_http_mark_ready(frame), ESP_OK); stop_error = ESP_FAIL;
    EXPECT_EQ(read_pico_http_stop(frame), ESP_FAIL);
    EXPECT_FALSE(read_pico_http_active(frame)); EXPECT_FALSE(read_pico_http_accepts(frame, handle));
    EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE);
    stop_error = ESP_OK;
    ASSERT_EQ(read_pico_http_stop(frame), ESP_OK);
    EXPECT_EQ(closed, (std::vector<httpd_handle_t>{handle, handle}));
    EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
    EXPECT_EQ(read_pico_http_claim(provision), ESP_OK);
}
TEST_F(ReadPicoHttpService, UnexpectedHandleOnFailedStartLatchesWithoutBlindClose) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK); start_error = ESP_FAIL; unexpected_handle = true;
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_FAIL);
    EXPECT_EQ(registrations, 0u); EXPECT_EQ(read_pico_http_stop(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_claim(provision), ESP_ERR_INVALID_STATE); EXPECT_EQ(stops, 0u);
}
TEST_F(ReadPicoHttpService, SuccessWithNoHandleCannotPublishOrReleaseUnprovenResources) {
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK); null_success = true;
    EXPECT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_mark_ready(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_stop(frame), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_http_release(frame), ESP_ERR_INVALID_STATE); EXPECT_EQ(stops, 0u);
}
TEST_F(ReadPicoHttpService, IdleStopAndReleaseDoNotTouchSdkAndNextStartRequiresFreshReadiness) {
    EXPECT_EQ(read_pico_http_stop(frame), ESP_OK); EXPECT_EQ(read_pico_http_release(frame), ESP_OK);
    EXPECT_EQ(stops, 0u); ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    ASSERT_EQ(read_pico_http_mark_ready(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_stop(frame), ESP_OK); ASSERT_EQ(read_pico_http_release(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_claim(frame), ESP_OK);
    ASSERT_EQ(read_pico_http_start(frame, &configuration, routes), ESP_OK);
    EXPECT_FALSE(read_pico_http_accepts(frame, handle));
}
