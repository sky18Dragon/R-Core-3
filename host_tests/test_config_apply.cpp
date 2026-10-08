#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
extern "C" {
#include "config_apply.h"
}
namespace {
esp_err_t count(void *context) { ++*static_cast<int *>(context); return ESP_OK; }
esp_err_t fail(void *context) { ++*static_cast<int *>(context); return ESP_FAIL; }
esp_err_t nested(void *context) {
    EXPECT_EQ(config_apply_run(count, context), ESP_ERR_INVALID_STATE);
    return ESP_ERR_NO_MEM;
}
struct Wait { std::atomic<bool> entered{false}, release{false}; };
esp_err_t wait(void *context) {
    auto *state = static_cast<Wait *>(context); state->entered = true;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!state->release.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    return ESP_OK;
}
}
TEST(ConfigApply, SuccessfulAndFailedCallbacksReleaseControlAndReturnPrimaryResult) {
    int calls = 0;
    EXPECT_EQ(config_apply_run(count, &calls), ESP_OK);
    EXPECT_EQ(config_apply_run(fail, &calls), ESP_FAIL);
    EXPECT_EQ(config_apply_run(count, &calls), ESP_OK); EXPECT_EQ(calls, 3);
}
TEST(ConfigApply, ReentrantRequestDoesNotRunAndOuterFailureDoesNotLatchControl) {
    int calls = 0;
    EXPECT_EQ(config_apply_run(nested, &calls), ESP_ERR_NO_MEM); EXPECT_EQ(calls, 0);
    EXPECT_EQ(config_apply_run(count, &calls), ESP_OK); EXPECT_EQ(calls, 1);
}
TEST(ConfigApply, InvalidCallbackDoesNotAcquireOrLatchControl) {
    EXPECT_EQ(config_apply_run(nullptr, nullptr), ESP_ERR_INVALID_ARG);
    int calls = 0; EXPECT_EQ(config_apply_run(count, &calls), ESP_OK); EXPECT_EQ(calls, 1);
}
TEST(ConfigApply, ActualConcurrentRequestReturnsBusyWithoutCallingAdapter) {
    Wait state; esp_err_t result = ESP_FAIL;
    std::thread worker([&] { result = config_apply_run(wait, &state); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!state.entered.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_TRUE(state.entered.load()); int calls = 0;
    EXPECT_EQ(config_apply_run(count, &calls), ESP_ERR_INVALID_STATE); EXPECT_EQ(calls, 0);
    state.release = true; worker.join(); EXPECT_EQ(result, ESP_OK);
    EXPECT_EQ(config_apply_run(count, &calls), ESP_OK); EXPECT_EQ(calls, 1);
}

namespace {
struct Transaction {
    int applies = 0, finishes = 0, nested_calls = 0;
    esp_err_t apply_error = ESP_OK, finish_error = ESP_OK;
};
esp_err_t apply_step(void *context) {
    auto *s = static_cast<Transaction *>(context); ++s->applies;
    EXPECT_EQ(config_apply_run(count, &s->nested_calls), ESP_ERR_INVALID_STATE);
    return s->apply_error;
}
esp_err_t finish_step(void *context) {
    auto *s = static_cast<Transaction *>(context); ++s->finishes;
    EXPECT_EQ(config_apply_run(count, &s->nested_calls), ESP_ERR_INVALID_STATE);
    return s->finish_error;
}
}
TEST(ConfigApply, TransactionProtectsApplyAndFinishWithoutRecursiveAdmission) {
    Transaction s; config_apply_result_t result{};
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_OK);
    EXPECT_TRUE(result.entered); EXPECT_TRUE(result.applied); EXPECT_TRUE(result.finished);
    EXPECT_EQ(s.applies, 1); EXPECT_EQ(s.finishes, 1); EXPECT_EQ(s.nested_calls, 0);
    EXPECT_EQ(config_apply_run(count, &s.nested_calls), ESP_OK);
}
TEST(ConfigApply, ApplyFailureSkipsRevisionAndReleasesForRetry) {
    Transaction s; config_apply_result_t result{}; s.apply_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(result.entered); EXPECT_FALSE(result.applied); EXPECT_FALSE(result.finished);
    EXPECT_EQ(s.finishes, 0);
    s.apply_error = ESP_OK;
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_OK);
    EXPECT_TRUE(result.finished); EXPECT_EQ(s.finishes, 1);
}
TEST(ConfigApply, RevisionFailureReportsAppliedButNotFinishedAndDoesNotLatchControl) {
    Transaction s; config_apply_result_t result{}; s.finish_error = ESP_FAIL;
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_FAIL);
    EXPECT_TRUE(result.entered); EXPECT_TRUE(result.applied); EXPECT_FALSE(result.finished);
    EXPECT_EQ(config_apply_run(count, &s.nested_calls), ESP_OK);
}
TEST(ConfigApply, InvalidTransactionLeavesCallerResultAndControlUntouched) {
    Transaction s; config_apply_result_t result{true, true, true};
    EXPECT_EQ(config_apply_transaction(nullptr, finish_step, &s, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_apply_transaction(apply_step, nullptr, &s, &result), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_TRUE(result.entered); EXPECT_TRUE(result.finished); EXPECT_EQ(s.applies, 0);
    EXPECT_EQ(config_apply_run(count, &s.nested_calls), ESP_OK);
}
TEST(ConfigApply, ConcurrentTransactionBusyIsDistinguishedFromAdmittedCallbackFailure) {
    Wait state; std::thread worker([&] { EXPECT_EQ(config_apply_run(wait, &state), ESP_OK); });
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!state.entered.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
    EXPECT_TRUE(state.entered.load()); Transaction s; config_apply_result_t result{true, true, true};
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(result.entered); EXPECT_FALSE(result.applied); EXPECT_FALSE(result.finished);
    EXPECT_EQ(s.applies, 0); EXPECT_EQ(s.finishes, 0);
    state.release = true; worker.join();
    EXPECT_EQ(config_apply_transaction(apply_step, finish_step, &s, &result), ESP_OK);
}
