#include <gtest/gtest.h>
#include <functional>
#include <string>
#include <vector>
extern "C" {
#include "read_pico_background.h"
}

namespace {
struct Task { TaskFunction_t entry; void *argument; std::string name; uint32_t stack; unsigned priority; };
std::vector<Task> tasks;
std::function<void()> on_create, on_delay, on_publish, on_callback;
TickType_t tick;
TaskHandle_t current = reinterpret_cast<void *>(0x100);
unsigned creations, callbacks, publications, deletions, fail_at;
esp_err_t publish_error;
bool published;
const auto button = READ_PICO_BACKGROUND_BUTTON;
const auto rotation = READ_PICO_BACKGROUND_ROTATION;
const auto late = READ_PICO_BACKGROUND_LATE_WIFI;
const uint32_t button_bit = READ_PICO_BACKGROUND_BIT(button);

void callback(void *argument) {
    EXPECT_TRUE(read_pico_background_running()); EXPECT_TRUE(published);
    EXPECT_EQ(argument, reinterpret_cast<void *>(0x400));
    ++callbacks;
    if (on_callback) on_callback();
}
esp_err_t publish() {
    EXPECT_FALSE(read_pico_background_running()); ++publications;
    if (on_publish) on_publish();
    if (publish_error == ESP_OK) published = true;
    return publish_error;
}
esp_err_t add(read_pico_background_role_t role = button) {
    return read_pico_background_add(role, callback, reinterpret_cast<void *>(0x400), 8192, 5);
}
void run(unsigned index) {
    const Task task = tasks.at(index);
    const auto previous = current;
    current = reinterpret_cast<void *>(static_cast<uintptr_t>(0x200 + index));
    task.entry(task.argument);
    current = previous;
}
class ReadPicoBackground : public testing::Test {
    void SetUp() override {
        tasks.clear(); on_create = on_delay = on_publish = on_callback = {};
        tick = creations = callbacks = publications = deletions = fail_at = 0;
        publish_error = ESP_OK; published = false;
        // Every CTest case runs in a fresh process: no reset for fault-latched slots.
    }
};
}

extern "C" {
BaseType_t xTaskCreate(TaskFunction_t entry, const char *name, uint32_t stack, void *argument,
                      UBaseType_t priority, TaskHandle_t *handle) {
    EXPECT_EQ(handle, nullptr); EXPECT_FALSE(read_pico_background_running());
    ++creations;
    if (on_create) on_create();
    if (fail_at == creations) return 0;
    tasks.push_back({entry, argument, name, stack, priority});
    return pdPASS;
}
TaskHandle_t xTaskGetCurrentTaskHandle() { return current; }
TickType_t xTaskGetTickCount() { return tick; }
void vTaskDelay(TickType_t duration) {
    EXPECT_GT(duration, 0u); EXPECT_LE(duration, 100u);
    tick += duration;
    // One-shot hook; it may recursively run a different synthetic task.
    auto action = std::move(on_delay); on_delay = {};
    if (action) action();
}
void vTaskDelete(TaskHandle_t handle) { EXPECT_EQ(handle, nullptr); ++deletions; }
}

TEST_F(ReadPicoBackground, InvalidArgumentsDoNotCreateOrPublishOrCancel) {
    EXPECT_EQ(add(static_cast<read_pico_background_role_t>(-1)), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(add(READ_PICO_BACKGROUND_COUNT), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_add(button, nullptr, nullptr, 8192, 5), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_add(button, callback, nullptr, 0, 5), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_activate(0, publish), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_activate(READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_COUNT), publish),
              ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_activate(button_bit, nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_background_cancel_and_wait(5001), ESP_ERR_INVALID_ARG);
    EXPECT_TRUE(read_pico_background_preparing()); EXPECT_EQ(creations, 0u);
    EXPECT_EQ(publications, 0u); EXPECT_EQ(deletions, 0u);
}
TEST_F(ReadPicoBackground, RequiredMissingRoleDoesNotPublishAndDuplicateDoesNotOverwrite) {
    ASSERT_EQ(add(), ESP_OK);
    EXPECT_EQ(add(), ESP_ERR_INVALID_STATE); EXPECT_EQ(creations, 1u);
    EXPECT_EQ(read_pico_background_activate(button_bit | READ_PICO_BACKGROUND_BIT(rotation), publish),
              ESP_ERR_INVALID_STATE);
    EXPECT_EQ(publications, 0u); EXPECT_TRUE(read_pico_background_preparing());
    ASSERT_EQ(add(rotation), ESP_OK);
    ASSERT_EQ(read_pico_background_activate(button_bit | READ_PICO_BACKGROUND_BIT(rotation), publish), ESP_OK);
    run(0); run(1); EXPECT_EQ(callbacks, 2u); EXPECT_EQ(deletions, 2u);
}
TEST_F(ReadPicoBackground, CallbackWaitsUntilReadinessPublicationAndKeepsTaskContract) {
    ASSERT_EQ(add(), ESP_OK);
    EXPECT_EQ(tasks[0].name, "button_task"); EXPECT_EQ(tasks[0].stack, 8192u);
    EXPECT_EQ(tasks[0].priority, 5u);
    on_delay = [] { ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK); };
    run(0); EXPECT_EQ(tick, 100u); EXPECT_EQ(callbacks, 1u); EXPECT_EQ(deletions, 1u);
    EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(add(late), ESP_ERR_INVALID_STATE); EXPECT_EQ(creations, 1u);
    EXPECT_EQ(publications, 1u);
}
TEST_F(ReadPicoBackground, CancellationWhileWaitingSkipsCallbackAndNeverReusesSlot) {
    ASSERT_EQ(add(), ESP_OK);
    on_delay = [] { read_pico_background_cancel(); };
    run(0); EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 1u);
    EXPECT_FALSE(read_pico_background_running()); EXPECT_FALSE(read_pico_background_preparing());
    EXPECT_EQ(add(late), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK);
}
TEST_F(ReadPicoBackground, CreationFailureCancelsEarlierTasksWithoutCallingBusinessCode) {
    ASSERT_EQ(add(rotation), ESP_OK); fail_at = 2;
    EXPECT_EQ(add(), ESP_ERR_NO_MEM);
    EXPECT_EQ(tasks.size(), 1u); EXPECT_EQ(deletions, 0u);
    EXPECT_EQ(read_pico_background_activate(READ_PICO_BACKGROUND_BIT(rotation), publish), ESP_ERR_INVALID_STATE);
    run(0); EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 1u);
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK); EXPECT_EQ(publications, 0u);
}
TEST_F(ReadPicoBackground, FirstCreationFailureHasNoPendingWorkerAndNoAutomaticRetry) {
    fail_at = 1;
    EXPECT_EQ(add(), ESP_ERR_NO_MEM);
    EXPECT_EQ(add(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK);
    EXPECT_EQ(creations, 1u); EXPECT_EQ(deletions, 0u);
}
TEST_F(ReadPicoBackground, ReadinessFailureCancelsEveryCreatedCallbackAndPreservesError) {
    ASSERT_EQ(add(), ESP_OK); ASSERT_EQ(add(rotation), ESP_OK); publish_error = ESP_FAIL;
    EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_FAIL);
    run(0); run(1); EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 2u);
    EXPECT_EQ(publications, 1u); EXPECT_FALSE(published);
}
TEST_F(ReadPicoBackground, CancellationDuringPublicationCannotReactivateAndCallerMustWithdrawReadiness) {
    ASSERT_EQ(add(), ESP_OK); on_publish = [] { read_pico_background_cancel(); };
    EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(published); // External readiness is caller-owned; not magically rolled back.
    EXPECT_FALSE(read_pico_background_running()); run(0); EXPECT_EQ(callbacks, 0u);
}
TEST_F(ReadPicoBackground, ReentrantCreateAndPublishCannotMutateSlots) {
    auto reject = [] {
        EXPECT_EQ(add(rotation), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_ERR_INVALID_STATE);
    };
    on_create = reject; ASSERT_EQ(add(), ESP_OK); on_create = {};
    on_publish = reject; ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK);
    EXPECT_EQ(creations, 1u); run(0); EXPECT_EQ(callbacks, 1u);
}
TEST_F(ReadPicoBackground, CancelWaitDuringCreateIsBusyAndDoesNotClaimExit) {
    on_create = [] { EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_ERR_INVALID_STATE); };
    EXPECT_EQ(add(), ESP_ERR_INVALID_STATE); EXPECT_EQ(tasks.size(), 1u);
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_ERR_TIMEOUT);
    run(0); EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK); EXPECT_EQ(callbacks, 0u);
}
TEST_F(ReadPicoBackground, CancelWaitDuringPublishIsBusyAndCannotReactivate) {
    ASSERT_EQ(add(), ESP_OK);
    on_publish = [] { EXPECT_EQ(read_pico_background_cancel_and_wait(100), ESP_ERR_INVALID_STATE); };
    EXPECT_EQ(read_pico_background_activate(button_bit, publish), ESP_ERR_INVALID_STATE);
    run(0); EXPECT_EQ(callbacks, 0u); EXPECT_FALSE(read_pico_background_running());
}
TEST_F(ReadPicoBackground, BoundedWaitDoesNotDeletePendingTaskAndAllowsExplicitConfirmation) {
    ASSERT_EQ(add(), ESP_OK);
    EXPECT_EQ(read_pico_background_cancel_and_wait(250), ESP_ERR_TIMEOUT);
    EXPECT_EQ(tick, 250u); EXPECT_EQ(deletions, 0u);
    run(0); EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK);
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 1u);
}
TEST_F(ReadPicoBackground, WaitConfirmsOnlyAfterOtherCallbacksHaveReturned) {
    ASSERT_EQ(add(), ESP_OK); ASSERT_EQ(add(rotation), ESP_OK);
    on_delay = [] { run(0); run(1); };
    EXPECT_EQ(read_pico_background_cancel_and_wait(2000), ESP_OK);
    EXPECT_EQ(tick, 100u); EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 2u);
}
TEST_F(ReadPicoBackground, ExecutingOwnedCallerCanCancelAndWaitWithoutWaitingForItself) {
    ASSERT_EQ(add(), ESP_OK); ASSERT_EQ(add(rotation), ESP_OK);
    ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK);
    on_callback = [] {
        on_delay = [] { run(1); };
        EXPECT_EQ(read_pico_background_cancel_and_wait(2000), ESP_OK);
        EXPECT_FALSE(read_pico_background_running());
    };
    run(0); EXPECT_EQ(callbacks, 1u); EXPECT_EQ(deletions, 2u);
}
TEST_F(ReadPicoBackground, OwnedCallerIsNotMistakenForAllOtherPendingTasks) {
    ASSERT_EQ(add(), ESP_OK); ASSERT_EQ(add(rotation), ESP_OK);
    ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK);
    on_callback = [] { EXPECT_EQ(read_pico_background_cancel_and_wait(100), ESP_ERR_TIMEOUT); };
    run(0); EXPECT_EQ(deletions, 1u); run(1);
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK);
}
TEST_F(ReadPicoBackground, CompletedTaskHandleDoesNotExemptAnotherPendingWorker) {
    ASSERT_EQ(add(), ESP_OK); ASSERT_EQ(add(rotation), ESP_OK);
    ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK);
    run(0); current = reinterpret_cast<void *>(0x200); // Synthetic handle reuse.
    EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_ERR_TIMEOUT);
    run(1); EXPECT_EQ(read_pico_background_cancel_and_wait(0), ESP_OK);
}
TEST_F(ReadPicoBackground, TickWraparoundRetainsFiniteWaitBudget) {
    ASSERT_EQ(add(), ESP_OK); tick = UINT32_MAX - 50;
    const auto started = tick;
    EXPECT_EQ(read_pico_background_cancel_and_wait(250), ESP_ERR_TIMEOUT);
    EXPECT_EQ(static_cast<TickType_t>(tick - started), 250u); EXPECT_EQ(deletions, 0u);
}
TEST_F(ReadPicoBackground, QueuedWorkerAfterCancelAndActivationCannotEnterCallback) {
    ASSERT_EQ(add(), ESP_OK);
    ASSERT_EQ(read_pico_background_activate(button_bit, publish), ESP_OK);
    read_pico_background_cancel(); run(0);
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(deletions, 1u);
    EXPECT_EQ(add(late), ESP_ERR_INVALID_STATE);
}
