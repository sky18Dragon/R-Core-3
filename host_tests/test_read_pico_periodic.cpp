#include <gtest/gtest.h>
#include <ctime>
#include <functional>
#include <map>
#include <string>
extern "C" {
#include "periodic_tasks.h"
#include "read_pico_background.h"
#include "esp_timer.h"
#include "nvs.h"
}
namespace {
esp_timer_handle_t handle = reinterpret_cast<void *>(0x50);
esp_timer_create_args_t timer;
TaskFunction_t worker;
void *worker_argument;
TaskHandle_t current = reinterpret_cast<void *>(0x60);
TickType_t ticks;
std::map<std::string, int64_t> records;
std::function<void()> on_create, on_start, on_stop, on_delete, on_read, on_callback, on_delay;
esp_err_t create_error, start_error, stop_error, delete_error, open_error, read_error, set_error, commit_error, callback_error;
bool null_success, unexpected_handle, worker_fail;
time_t now;
unsigned creates, starts, stops, deletes, workers, self_deletes, opens, rw_opens, reads, closes, writes, commits, callbacks;
int64_t staged;
std::string staged_key;
esp_err_t callback() {
    EXPECT_TRUE(periodic_tasks_is_active()); ++callbacks;
    if (on_callback) on_callback();
    return callback_error;
}
esp_err_t publish() { return ESP_OK; }
void ready(const char *name = SNTP_TASK_NAME, uint32_t interval = 86400) {
    ASSERT_EQ(periodic_tasks_init(), ESP_OK);
    ASSERT_EQ(periodic_tasks_register(name, callback, interval), ESP_OK);
}
void emit() { timer.callback(timer.arg); }
void run_worker() {
    ASSERT_NE(worker, nullptr);
    const auto previous = current; current = reinterpret_cast<void *>(0x70);
    worker(worker_argument); current = previous;
}
class ReadPicoPeriodic : public testing::Test {
    void SetUp() override {
        create_error = start_error = stop_error = delete_error = open_error = read_error =
            set_error = commit_error = callback_error = ESP_OK;
        null_success = unexpected_handle = worker_fail = false;
        creates = starts = stops = deletes = workers = self_deletes = opens = rw_opens =
            reads = closes = writes = commits = callbacks = ticks = 0;
        now = 1800000000; records.clear(); worker = nullptr; timer = {};
        on_create = on_start = on_stop = on_delete = on_read = on_callback = on_delay = {};
        // Irreversible lifecycle/fault cases each run in a separate CTest process.
    }
};
}
extern "C" {
time_t read_pico_periodic_test_time(time_t *output) { if (output) *output = now; return now; }
const char *esp_err_to_name(esp_err_t) { return "synthetic error"; }
esp_err_t esp_timer_create(const esp_timer_create_args_t *args, esp_timer_handle_t *output) {
    EXPECT_EQ(*output, nullptr); ++creates; timer = *args;
    EXPECT_STREQ(args->name, "periodic_check_timer");
    if (on_create) on_create();
    if ((create_error == ESP_OK && !null_success) || unexpected_handle) *output = handle;
    return create_error;
}
esp_err_t esp_timer_start_periodic(esp_timer_handle_t received, uint64_t interval) {
    EXPECT_EQ(received, handle); EXPECT_EQ(interval, UINT64_C(3600000000)); ++starts;
    if (on_start) on_start(); return start_error;
}
esp_err_t esp_timer_stop_blocking(esp_timer_handle_t received, uint32_t timeout) {
    EXPECT_EQ(received, handle); EXPECT_EQ(timeout, 100u); ++stops;
    EXPECT_FALSE(periodic_tasks_is_active());
    if (on_stop) on_stop(); return stop_error;
}
esp_err_t esp_timer_delete(esp_timer_handle_t received) {
    EXPECT_EQ(received, handle); ++deletes;
    if (on_delete) on_delete(); return delete_error;
}
BaseType_t xTaskCreate(TaskFunction_t entry, const char *name, uint32_t stack, void *argument,
                      UBaseType_t priority, TaskHandle_t *output) {
    EXPECT_STREQ(name, "periodic_tasks"); EXPECT_EQ(stack, 8192u); EXPECT_EQ(priority, 5u);
    EXPECT_EQ(output, nullptr); ++workers;
    if (worker_fail) return 0;
    worker = entry; worker_argument = argument; return pdPASS;
}
TaskHandle_t xTaskGetCurrentTaskHandle() { return current; }
TickType_t xTaskGetTickCount() { return ticks; }
void vTaskDelay(TickType_t duration) {
    ticks += duration;
    auto action = std::move(on_delay); on_delay = {};
    if (action) action();
}
void vTaskDelete(TaskHandle_t received) { EXPECT_EQ(received, nullptr); ++self_deletes; }
esp_err_t nvs_open(const char *ns, int mode, nvs_handle_t *output) {
    EXPECT_STREQ(ns, "periodic"); ++opens;
    if (mode == NVS_READWRITE) ++rw_opens;
    if (open_error == ESP_OK) *output = 1;
    return open_error;
}
void nvs_close(nvs_handle_t received) { EXPECT_EQ(received, 1); ++closes; }
esp_err_t nvs_get_i64(nvs_handle_t received, const char *key, int64_t *output) {
    EXPECT_EQ(received, 1); ++reads; if (on_read) on_read();
    *output = -999; // SDK may change a local output even when it reports failure.
    if (read_error != ESP_OK) return read_error;
    auto found = records.find(key);
    if (found == records.end()) return ESP_ERR_NVS_NOT_FOUND;
    *output = found->second; return ESP_OK;
}
esp_err_t nvs_set_i64(nvs_handle_t received, const char *key, int64_t value) {
    EXPECT_EQ(received, 1); ++writes; staged = value; staged_key = key; return set_error;
}
esp_err_t nvs_commit(nvs_handle_t received) {
    EXPECT_EQ(received, 1); ++commits;
    if (commit_error == ESP_OK) records[staged_key] = staged;
    return commit_error;
}
// Any accidental erase reference makes this test target fail to link.
}

TEST_F(ReadPicoPeriodic, RepeatedInitDoesNotReplaceTimerOrClearRegistration) {
    ready(); ASSERT_EQ(periodic_tasks_init(), ESP_OK);
    EXPECT_EQ(creates, 1u); EXPECT_EQ(starts, 1u); EXPECT_EQ(workers, 1u);
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_OK); EXPECT_EQ(callbacks, 1u);
    EXPECT_EQ(records[SNTP_TASK_NAME], now);
}
TEST_F(ReadPicoPeriodic, CreateFailureDoesNotGuessHandleOrAutomaticallyRetry) {
    create_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(starts, 0u); EXPECT_EQ(stops, 0u); EXPECT_EQ(deletes, 0u); EXPECT_EQ(workers, 0u);
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
}
TEST_F(ReadPicoPeriodic, NullSuccessfulCreateLatchesUncertainWithoutDeletingUnknownObject) {
    null_success = true;
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(periodic_tasks_stop(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(stops, 0u); EXPECT_EQ(deletes, 0u); EXPECT_EQ(workers, 0u);
}
TEST_F(ReadPicoPeriodic, ErrorWithUnexpectedHandleDoesNotBlindlyStopOrDelete) {
    create_error = ESP_FAIL; unexpected_handle = true;
    EXPECT_EQ(periodic_tasks_init(), ESP_FAIL);
    EXPECT_EQ(periodic_tasks_stop(), ESP_ERR_INVALID_STATE); EXPECT_EQ(stops, 0u);
}
TEST_F(ReadPicoPeriodic, StartFailureClosesExactCreatedTimerAndPreservesPrimaryError) {
    start_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_init(), ESP_FAIL); EXPECT_EQ(workers, 0u);
    EXPECT_EQ(stops, 1u); EXPECT_EQ(deletes, 1u);
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK); EXPECT_EQ(stops, 1u);
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoPeriodic, FailedStartupCleanupRetainsHandleForExplicitStop) {
    start_error = ESP_ERR_NO_MEM; stop_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_NO_MEM);
    EXPECT_EQ(stops, 1u); EXPECT_EQ(deletes, 0u);
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
    stop_error = ESP_OK; EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
    EXPECT_EQ(stops, 2u); EXPECT_EQ(deletes, 1u);
}
TEST_F(ReadPicoPeriodic, WorkerCreationFailureCancelsGroupAndChecksTimerCleanup) {
    worker_fail = true;
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_NO_MEM);
    EXPECT_FALSE(read_pico_background_running()); EXPECT_FALSE(read_pico_background_preparing());
    EXPECT_EQ(stops, 1u); EXPECT_EQ(deletes, 1u); EXPECT_EQ(callbacks, 0u);
}
TEST_F(ReadPicoPeriodic, ReentrantSdkOperationsCannotMutateOrReinitializeService) {
    auto reject = [] {
        EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_register(SNTP_TASK_NAME, callback, 10), ESP_ERR_INVALID_STATE);
    };
    on_create = on_start = reject; ready();
    on_stop = on_delete = reject;
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK); EXPECT_EQ(creates, 1u);
}
TEST_F(ReadPicoPeriodic, StopRequestedDuringCreateDoesNotStartOrReviveService) {
    on_create = [] { periodic_tasks_request_stop(); };
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE); EXPECT_EQ(starts, 0u);
    EXPECT_EQ(stops, 1u); EXPECT_EQ(deletes, 1u); EXPECT_EQ(workers, 0u);
}
TEST_F(ReadPicoPeriodic, StopRequestedDuringStartCannotReviveTheWorker) {
    on_start = [] { periodic_tasks_request_stop(); };
    EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
    read_pico_background_cancel(); run_worker();
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(self_deletes, 1u); EXPECT_EQ(deletes, 1u);
}
TEST_F(ReadPicoPeriodic, TimerDispatchOnlyQueuesAndDoesNoBusinessIo) {
    ready(); emit(); emit();
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(opens, 0u); EXPECT_EQ(writes, 0u);
    ASSERT_EQ(read_pico_background_activate(READ_PICO_BACKGROUND_BIT(READ_PICO_BACKGROUND_PERIODIC), publish), ESP_OK);
    on_callback = [] { on_delay = [] { periodic_tasks_request_stop(); }; };
    run_worker();
    EXPECT_EQ(callbacks, 1u); EXPECT_EQ(commits, 1u); EXPECT_EQ(self_deletes, 1u);
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
}
TEST_F(ReadPicoPeriodic, TimerDuringInitializationOrAfterStopCannotRunBusinessCode) {
    on_start = [] { emit(); }; ready();
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK); emit();
    read_pico_background_cancel(); run_worker();
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(opens, 0u);
}
TEST_F(ReadPicoPeriodic, StopFailureKeepsExactTimerAndNeverAutomaticallyRestarts) {
    ready(); stop_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(periodic_tasks_stop(), ESP_ERR_TIMEOUT); EXPECT_EQ(deletes, 0u);
    EXPECT_FALSE(periodic_tasks_is_active()); EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
    stop_error = ESP_OK; EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
    EXPECT_EQ(stops, 2u); EXPECT_EQ(deletes, 1u);
    EXPECT_EQ(periodic_tasks_stop(), ESP_OK); EXPECT_EQ(stops, 2u);
}
TEST_F(ReadPicoPeriodic, DeleteFailureRetainsTimerUntilExplicitConfirmedRetry) {
    ready(); delete_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_stop(), ESP_FAIL);
    delete_error = ESP_OK; EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
    EXPECT_EQ(stops, 2u); EXPECT_EQ(deletes, 2u);
}
TEST_F(ReadPicoPeriodic, InvalidKeysAndUninitializedOperationsDoNotTouchNvsOrTimer) {
    bool result = true; int64_t output = 42;
    EXPECT_EQ(periodic_tasks_register(nullptr, callback, 1), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(periodic_tasks_register("", callback, 1), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(periodic_tasks_register("1234567890123456", callback, 1), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(periodic_tasks_register(SNTP_TASK_NAME, nullptr, 1), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(periodic_tasks_register(SNTP_TASK_NAME, callback, 1), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(periodic_tasks_force_run(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(periodic_tasks_should_run_checked(SNTP_TASK_NAME, &result), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &output), ESP_ERR_INVALID_STATE);
    EXPECT_TRUE(result); EXPECT_EQ(output, 42); EXPECT_EQ(opens, 0u); EXPECT_EQ(creates, 0u);
}
TEST_F(ReadPicoPeriodic, FullRegistryStillAllowsUpdatingAnExistingTask) {
    ASSERT_EQ(periodic_tasks_init(), ESP_OK);
    for (unsigned i = 0; i < 16; ++i)
        ASSERT_EQ(periodic_tasks_register(("task" + std::to_string(i)).c_str(), callback, 10), ESP_OK);
    EXPECT_EQ(periodic_tasks_register("overflow", callback, 10), ESP_ERR_NO_MEM);
    EXPECT_EQ(periodic_tasks_register("task0", callback, 0), ESP_OK);
    EXPECT_FALSE(periodic_tasks_should_run("task0"));
    EXPECT_EQ(opens, 0u); EXPECT_EQ(periodic_tasks_force_run("task0"), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoPeriodic, MissingValidTimestampRunsThenUsesConfirmedCompletionInterval) {
    ready(); EXPECT_TRUE(periodic_tasks_should_run(SNTP_TASK_NAME));
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_OK); EXPECT_EQ(callbacks, 1u);
    EXPECT_EQ(records[SNTP_TASK_NAME], now);
    EXPECT_FALSE(periodic_tasks_should_run(SNTP_TASK_NAME));
    now += 86400; EXPECT_TRUE(periodic_tasks_should_run(SNTP_TASK_NAME));
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_OK); EXPECT_EQ(callbacks, 2u);
}
TEST_F(ReadPicoPeriodic, NamespaceMissingIsNotTheSameAsAnIoError) {
    ready(); open_error = ESP_ERR_NVS_NOT_FOUND;
    EXPECT_TRUE(periodic_tasks_should_run(SNTP_TASK_NAME));
    open_error = ESP_FAIL;
    EXPECT_FALSE(periodic_tasks_should_run(SNTP_TASK_NAME));
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_FAIL); EXPECT_EQ(callbacks, 0u);
}
TEST_F(ReadPicoPeriodic, CorruptOrWrongTypeRecordDoesNotRunOrChangeCallerOutput) {
    ready(); read_error = ESP_ERR_NVS_TYPE_MISMATCH;
    bool output = true; int64_t last = 42;
    EXPECT_EQ(periodic_tasks_should_run_checked(SNTP_TASK_NAME, &output), ESP_ERR_NVS_TYPE_MISMATCH);
    EXPECT_TRUE(output);
    EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &last), ESP_ERR_NVS_TYPE_MISMATCH);
    EXPECT_EQ(last, 42); EXPECT_EQ(periodic_tasks_check_and_run(), ESP_ERR_NVS_TYPE_MISMATCH);
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(writes, 0u); EXPECT_EQ(closes, reads);
}
TEST_F(ReadPicoPeriodic, InvalidStoredTimeIsRejectedWithoutAutomaticErase) {
    ready(); records[SNTP_TASK_NAME] = -1;
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(records[SNTP_TASK_NAME], -1); EXPECT_EQ(callbacks, 0u); EXPECT_EQ(rw_opens, 0u);
}
TEST_F(ReadPicoPeriodic, InvalidClockDoesNotRunNormallyButExplicitBootstrapCanCorrectIt) {
    ready(); now = 0;
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_ERR_INVALID_STATE); EXPECT_EQ(callbacks, 0u);
    on_callback = [] { now = 1800000000; };
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_OK);
    EXPECT_EQ(callbacks, 1u); EXPECT_EQ(records[SNTP_TASK_NAME], now);
}
TEST_F(ReadPicoPeriodic, FailedTimeReadCannotCauseAnInvalidNvsWrite) {
    ready(); now = -1;
    EXPECT_EQ(periodic_tasks_update_last_run(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(rw_opens, 0u); EXPECT_EQ(writes, 0u);
}
TEST_F(ReadPicoPeriodic, BackwardClockAndExtremeFutureTimestampDoNotOverflowOrRun) {
    ready(); records[SNTP_TASK_NAME] = INT64_MAX;
    EXPECT_FALSE(periodic_tasks_should_run(SNTP_TASK_NAME));
    records[SNTP_TASK_NAME] = now + 60;
    EXPECT_FALSE(periodic_tasks_should_run(SNTP_TASK_NAME));
}
TEST_F(ReadPicoPeriodic, ForceIsVolatileAndCannotBypassNvsReadErrors) {
    ready(); records[SNTP_TASK_NAME] = now;
    EXPECT_EQ(periodic_tasks_force_run(SNTP_TASK_NAME), ESP_OK);
    EXPECT_EQ(opens, 0u); EXPECT_EQ(writes, 0u); EXPECT_EQ(commits, 0u);
    read_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_FAIL); EXPECT_EQ(callbacks, 0u);
    read_error = ESP_OK;
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_OK); EXPECT_EQ(callbacks, 1u);
}
TEST_F(ReadPicoPeriodic, ManualRunIsOneAtomicTaskAndNotAnUnrelatedBatch) {
    ready(); ASSERT_EQ(periodic_tasks_register("other", callback, 10), ESP_OK);
    records[SNTP_TASK_NAME] = now;
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_OK);
    EXPECT_EQ(callbacks, 1u); EXPECT_EQ(records.count("other"), 0u);
}
TEST_F(ReadPicoPeriodic, CallbackFailurePropagatesAndLeavesPriorTimestampUnchanged) {
    ready(); records[SNTP_TASK_NAME] = now - 86400; callback_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(writes, 0u); EXPECT_EQ(records[SNTP_TASK_NAME], now - 86400);
    callback_error = ESP_OK; EXPECT_EQ(periodic_tasks_check_and_run(), ESP_OK); EXPECT_EQ(callbacks, 2u);
}
TEST_F(ReadPicoPeriodic, NvsOpenWriteFailureLatchesUnconfirmedCompletionWithoutRerunningCallback) {
    ready(); on_callback = [] { open_error = ESP_FAIL; };
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_FAIL); EXPECT_EQ(callbacks, 1u);
    open_error = ESP_OK;
    EXPECT_EQ(periodic_tasks_force_run(SNTP_TASK_NAME), ESP_FAIL);
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_FAIL); EXPECT_EQ(callbacks, 1u);
}
TEST_F(ReadPicoPeriodic, NvsSetFailureClosesHandleAndDoesNotCommitOrRerun) {
    ready(); set_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_FAIL);
    EXPECT_EQ(commits, 0u); EXPECT_EQ(closes, opens);
    set_error = ESP_OK; EXPECT_EQ(periodic_tasks_check_and_run(), ESP_FAIL); EXPECT_EQ(callbacks, 1u);
}
TEST_F(ReadPicoPeriodic, CommitFailureCannotBeClearedByInitRegisterOrForce) {
    ready(); commit_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_FAIL); EXPECT_EQ(callbacks, 1u);
    commit_error = ESP_OK; EXPECT_EQ(periodic_tasks_init(), ESP_OK);
    EXPECT_EQ(periodic_tasks_register(SNTP_TASK_NAME, callback, 10), ESP_OK);
    EXPECT_EQ(periodic_tasks_force_run(SNTP_TASK_NAME), ESP_FAIL);
    EXPECT_EQ(periodic_tasks_update_last_run(SNTP_TASK_NAME), ESP_FAIL);
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_FAIL); EXPECT_EQ(callbacks, 1u); EXPECT_EQ(commits, 1u);
}
TEST_F(ReadPicoPeriodic, EveryReentrantBatchAndRegistryOrRecordMutationIsRejected) {
    ready(); on_callback = [] {
        bool output = true; int64_t last = 42;
        EXPECT_EQ(periodic_tasks_check_and_run(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_force_run(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_register("other", callback, 1), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_init(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_should_run_checked(SNTP_TASK_NAME, &output), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &last), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(periodic_tasks_update_last_run(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
        EXPECT_TRUE(output); EXPECT_EQ(last, 42);
    };
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_OK); EXPECT_EQ(callbacks, 1u);
}
TEST_F(ReadPicoPeriodic, StopInsideCallbackClosesAdmissionButDoesNotDeleteInUseTimerOrPersist) {
    ready(); on_callback = [] {
        EXPECT_EQ(periodic_tasks_stop(), ESP_ERR_INVALID_STATE);
        EXPECT_EQ(stops, 0u); EXPECT_FALSE(periodic_tasks_is_active());
    };
    EXPECT_EQ(periodic_tasks_run_now(SNTP_TASK_NAME), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(writes, 0u); EXPECT_EQ(periodic_tasks_stop(), ESP_OK); EXPECT_EQ(deletes, 1u);
}
TEST_F(ReadPicoPeriodic, StopDuringReadDoesNotEnterCallbackAndOutputsRemainUnchanged) {
    ready(); on_read = [] { periodic_tasks_request_stop(); };
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(callbacks, 0u); EXPECT_EQ(writes, 0u); EXPECT_EQ(periodic_tasks_stop(), ESP_OK);
}
TEST_F(ReadPicoPeriodic, ReadOutputIsUnchangedOnMissingNamespaceAndInvalidRecord) {
    ready(); int64_t output = 42;
    open_error = ESP_ERR_NVS_NOT_FOUND;
    EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &output), ESP_ERR_NVS_NOT_FOUND);
    EXPECT_EQ(output, 42); open_error = ESP_OK; records[SNTP_TASK_NAME] = 0;
    EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &output), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(output, 42);
}

TEST_F(ReadPicoPeriodic, StopDuringDueReadDoesNotPublishAStaleTrueResult) {
    ready(); bool output = false;
    on_read = [] { periodic_tasks_request_stop(); };
    EXPECT_EQ(periodic_tasks_should_run_checked(SNTP_TASK_NAME, &output), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(output); EXPECT_FALSE(periodic_tasks_should_run(SNTP_TASK_NAME));
}
TEST_F(ReadPicoPeriodic, StopDuringLastReadDoesNotPublishAStaleTimestamp) {
    ready(); records[SNTP_TASK_NAME] = now; int64_t output = 42;
    on_read = [] { periodic_tasks_request_stop(); };
    EXPECT_EQ(periodic_tasks_get_last_run(SNTP_TASK_NAME, &output), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(output, 42);
}
TEST_F(ReadPicoPeriodic, FailedFirstBatchTaskDoesNotContinueIntoAnotherCallback) {
    ready(); ASSERT_EQ(periodic_tasks_register("other", callback, 10), ESP_OK);
    callback_error = ESP_FAIL;
    EXPECT_EQ(periodic_tasks_check_and_run(), ESP_FAIL);
    EXPECT_EQ(callbacks, 1u); EXPECT_EQ(records.count("other"), 0u);
}
