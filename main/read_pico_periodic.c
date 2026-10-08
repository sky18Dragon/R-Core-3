#include "periodic_tasks.h"

#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "read_pico_background.h"

#define MAX_TASKS 16
#define CHECK_INTERVAL_US (UINT64_C(3600) * 1000000)
#define FIRST_VALID_TIME INT64_C(1577836800)
static const char *TAG = "periodic_tasks";
static const char *NAMESPACE = "periodic";
typedef enum { IDLE, INITIALIZING, ACTIVE, FAILED, UNCERTAIN, STOPPED } phase_t;
typedef struct {
    char name[16]; // ESP-IDF NVS key limit: 15 bytes plus NUL.
    periodic_task_callback_t callback;
    uint32_t interval;
    bool forced;
    esp_err_t record_fault; // Callback ran, but completion persistence was not confirmed.
} periodic_task_t;

static periodic_task_t s_tasks[MAX_TASKS];
static unsigned s_count;
static esp_timer_handle_t s_timer;
static atomic_int s_phase = IDLE;
static atomic_bool s_stop_requested;
static atomic_bool s_due;
static atomic_flag s_control = ATOMIC_FLAG_INIT;
static esp_err_t check_and_run(bool *busy);

bool periodic_tasks_is_active(void)
{
    return atomic_load(&s_phase) == ACTIVE && !atomic_load(&s_stop_requested);
}
void periodic_tasks_request_stop(void) { atomic_store(&s_stop_requested, true); }

static bool valid_name(const char *name)
{
    return name && name[0] && strnlen(name, sizeof(s_tasks[0].name)) < sizeof(s_tasks[0].name);
}
static periodic_task_t *find_task(const char *name)
{
    for (unsigned i = 0; i < s_count; ++i)
        if (!strcmp(name, s_tasks[i].name)) return &s_tasks[i];
    return NULL;
}

static void timer_callback(void *argument)
{
    // Shared ESP_TIMER_TASK: no NVS, logs, delays, network or business callback.
    if (periodic_tasks_is_active()) atomic_store(&s_due, true);
}
static void periodic_worker(void *argument)
{
    while (read_pico_background_running() && periodic_tasks_is_active()) {
        TickType_t delay = pdMS_TO_TICKS(100);
        vTaskDelay(delay ? delay : 1);
        if (!read_pico_background_running() || !periodic_tasks_is_active()) return;
        if (atomic_exchange(&s_due, false)) {
            bool busy = false;
            esp_err_t error = check_and_run(&busy);
            if (busy && periodic_tasks_is_active()) atomic_store(&s_due, true);
            else if (error != ESP_OK) ESP_LOGW(TAG, "Periodic batch not completed: %s", esp_err_to_name(error));
        }
    }
}

// Caller owns s_control; stop_blocking is called from user tasks, never this
// adapter's timer callback. Successful delete schedules SDK memory reclamation.
static esp_err_t close_timer(void)
{
    if (!s_timer) return ESP_OK;
    esp_err_t error = esp_timer_stop_blocking(s_timer, pdMS_TO_TICKS(100));
    if (error != ESP_OK) return error;
    error = esp_timer_delete(s_timer);
    if (error == ESP_OK) s_timer = NULL;
    return error;
}

esp_err_t periodic_tasks_init(void)
{
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (periodic_tasks_is_active()) { result = ESP_OK; goto done; }
    if (atomic_load(&s_phase) != IDLE || atomic_load(&s_stop_requested) ||
        !read_pico_background_preparing()) goto done;
    atomic_store(&s_phase, INITIALIZING);
    const esp_timer_create_args_t args = {.callback = timer_callback, .name = "periodic_check_timer"};
    esp_timer_handle_t created = NULL;
    result = esp_timer_create(&args, &created);
    if ((result == ESP_OK && !created) || (result != ESP_OK && created)) {
        atomic_store(&s_phase, UNCERTAIN); // Do not guess ownership of an anomalous SDK output.
        if (result == ESP_OK) result = ESP_ERR_INVALID_STATE;
        goto done;
    }
    if (result != ESP_OK) { atomic_store(&s_phase, FAILED); goto done; }
    s_timer = created;
    if (atomic_load(&s_stop_requested)) result = ESP_ERR_INVALID_STATE;
    else result = esp_timer_start_periodic(s_timer, CHECK_INTERVAL_US);
    if (result == ESP_OK)
        result = read_pico_background_add(READ_PICO_BACKGROUND_PERIODIC, periodic_worker, NULL, 8192, 5);
    if (result == ESP_OK && atomic_load(&s_stop_requested)) result = ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) atomic_store(&s_phase, ACTIVE);
    else {
        atomic_store(&s_phase, FAILED);
        esp_err_t cleanup = close_timer();
        if (cleanup != ESP_OK) ESP_LOGE(TAG, "Periodic startup cleanup unconfirmed: %s", esp_err_to_name(cleanup));
    }
done:
    atomic_flag_clear(&s_control);
    return result;
}

esp_err_t periodic_tasks_stop(void)
{
    periodic_tasks_request_stop();
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (atomic_load(&s_phase) != UNCERTAIN) {
        result = close_timer();
        atomic_store(&s_phase, result == ESP_OK ? STOPPED : FAILED);
        atomic_store(&s_due, false);
    }
    atomic_flag_clear(&s_control);
    return result;
}

esp_err_t periodic_tasks_register(const char *name, periodic_task_callback_t callback, uint32_t interval)
{
    if (!valid_name(name) || !callback) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (periodic_tasks_is_active()) {
        periodic_task_t *task = find_task(name);
        if (!task && s_count == MAX_TASKS) result = ESP_ERR_NO_MEM;
        else {
            if (!task) {
                task = &s_tasks[s_count++];
                memcpy(task->name, name, strlen(name) + 1);
            }
            // Updating a callback does not clear an uncertain completion record.
            task->callback = callback;
            task->interval = interval;
            result = ESP_OK;
        }
    }
    atomic_flag_clear(&s_control);
    return result;
}

static esp_err_t read_last_run(const char *name, int64_t *output)
{
    nvs_handle_t handle;
    esp_err_t error = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (error != ESP_OK) return error;
    int64_t value = 0;
    error = nvs_get_i64(handle, name, &value);
    nvs_close(handle);
    if (error != ESP_OK) return error;
    if (value < FIRST_VALID_TIME) return ESP_ERR_INVALID_STATE;
    *output = value;
    return ESP_OK;
}
static esp_err_t current_time(int64_t *output)
{
    time_t now = time(NULL);
    if (now == (time_t) -1 || (int64_t) now < FIRST_VALID_TIME) return ESP_ERR_INVALID_STATE;
    *output = (int64_t) now;
    return ESP_OK;
}
static esp_err_t due(periodic_task_t *task, bool *output)
{
    if (task->record_fault != ESP_OK) return task->record_fault;
    if (!task->interval) { *output = false; return ESP_OK; }
    int64_t previous = 0;
    esp_err_t error = read_last_run(task->name, &previous);
    if (error != ESP_OK && error != ESP_ERR_NVS_NOT_FOUND) return error;
    // Even a forced request must not bypass a corrupted/unreadable record.
    if (task->forced) { *output = true; return ESP_OK; }
    int64_t now = 0;
    esp_err_t clock = current_time(&now);
    if (clock != ESP_OK) return clock;
    *output = error == ESP_ERR_NVS_NOT_FOUND ||
              (now >= previous && now - previous >= (int64_t) task->interval);
    return ESP_OK;
}

esp_err_t periodic_tasks_should_run_checked(const char *name, bool *output)
{
    if (!valid_name(name) || !output) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    periodic_task_t *task = find_task(name);
    bool local = false;
    if (periodic_tasks_is_active()) result = task ? due(task, &local) : ESP_ERR_NOT_FOUND;
    if (result == ESP_OK && !periodic_tasks_is_active()) result = ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) *output = local;
    atomic_flag_clear(&s_control);
    return result;
}
bool periodic_tasks_should_run(const char *name)
{
    bool result = false;
    esp_err_t error = periodic_tasks_should_run_checked(name, &result);
    if (error != ESP_OK) ESP_LOGW(TAG, "Periodic due check refused: %s", esp_err_to_name(error));
    return result;
}

static esp_err_t store_last_run(const char *name)
{
    int64_t now;
    esp_err_t error = current_time(&now);
    if (error != ESP_OK) return error;
    if (!periodic_tasks_is_active()) return ESP_ERR_INVALID_STATE;
    nvs_handle_t handle;
    error = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK) return error;
    error = nvs_set_i64(handle, name, now);
    if (error == ESP_OK) error = nvs_commit(handle);
    nvs_close(handle);
    return error;
}
esp_err_t periodic_tasks_update_last_run(const char *name)
{
    if (!valid_name(name)) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    periodic_task_t *task = find_task(name);
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (periodic_tasks_is_active()) {
        if (!task) result = ESP_ERR_NOT_FOUND;
        else if (task->record_fault) result = task->record_fault;
        else {
            result = store_last_run(name);
            if (result != ESP_OK) task->record_fault = result;
        }
    }
    atomic_flag_clear(&s_control);
    return result;
}
esp_err_t periodic_tasks_get_last_run(const char *name, int64_t *output)
{
    if (!valid_name(name) || !output) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    int64_t local = 0;
    esp_err_t result = periodic_tasks_is_active() ? read_last_run(name, &local) : ESP_ERR_INVALID_STATE;
    if (result == ESP_OK && !periodic_tasks_is_active()) result = ESP_ERR_INVALID_STATE;
    if (result == ESP_OK) *output = local;
    atomic_flag_clear(&s_control);
    return result;
}
esp_err_t periodic_tasks_force_run(const char *name)
{
    if (!valid_name(name)) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    periodic_task_t *task = find_task(name);
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (periodic_tasks_is_active()) {
        if (!task) result = ESP_ERR_NOT_FOUND;
        else if (task->record_fault) result = task->record_fault;
        else if (task->interval) { task->forced = true; result = ESP_OK; }
    }
    atomic_flag_clear(&s_control);
    return result; // No NVS erase or commit to request a run.
}
static esp_err_t run_task(periodic_task_t *task)
{
    if (!periodic_tasks_is_active()) return ESP_ERR_INVALID_STATE;
    bool run = false;
    esp_err_t result = due(task, &run);
    if (result != ESP_OK || !run) return result;
    if (!periodic_tasks_is_active()) return ESP_ERR_INVALID_STATE;
    result = task->callback();
    if (result != ESP_OK) return result;
    if (!periodic_tasks_is_active()) return ESP_ERR_INVALID_STATE;
    result = store_last_run(task->name);
    if (result != ESP_OK) {
        task->record_fault = result;
        ESP_LOGE(TAG, "Task completed, timestamp unconfirmed: %s", esp_err_to_name(result));
    } else {
        if (!periodic_tasks_is_active()) return ESP_ERR_INVALID_STATE;
        task->forced = false;
        ESP_LOGI(TAG, "Task completion and timestamp confirmed");
    }
    return result;
}
esp_err_t periodic_tasks_run_now(const char *name)
{
    if (!valid_name(name)) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    periodic_task_t *task = find_task(name);
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (periodic_tasks_is_active()) {
        if (!task) result = ESP_ERR_NOT_FOUND;
        else if (task->record_fault) result = task->record_fault;
        else if (task->interval) { task->forced = true; result = run_task(task); }
    }
    atomic_flag_clear(&s_control);
    return result;
}
static esp_err_t check_and_run(bool *busy)
{
    if (atomic_flag_test_and_set(&s_control)) {
        if (busy) *busy = true;
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t result = periodic_tasks_is_active() ? ESP_OK : ESP_ERR_INVALID_STATE;
    for (unsigned i = 0; result == ESP_OK && i < s_count; ++i) {
        result = run_task(&s_tasks[i]);
    }
    atomic_flag_clear(&s_control);
    return result;
}
esp_err_t periodic_tasks_check_and_run(void) { return check_and_run(NULL); }
