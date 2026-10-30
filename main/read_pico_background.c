#include "read_pico_background.h"

#include <stddef.h>
#include <stdatomic.h>

typedef enum { PREPARING, RUNNING, CANCELLED } phase_t;
typedef struct {
    TaskFunction_t callback;
    void *argument;
    _Atomic(TaskHandle_t) caller;
    atomic_bool executing;
} worker_t;

static worker_t s_workers[READ_PICO_BACKGROUND_COUNT];
static atomic_int s_phase = PREPARING;
static atomic_uint s_pending;
static atomic_flag s_control = ATOMIC_FLAG_INIT;
// Only accessed under s_control. All slot storage lives for the whole boot.
static uint32_t s_created;
static const char *const s_names[] = {
    "sleep_timer", "rotation_timer", "button_task", "late_wifi", "periodic_tasks"
};

static TickType_t poll_ticks(void)
{
    TickType_t ticks = pdMS_TO_TICKS(100);
    return ticks ? ticks : 1;
}

static void worker_entry(void *argument)
{
    worker_t *worker = argument;
    atomic_store(&worker->caller, xTaskGetCurrentTaskHandle());
    while (atomic_load(&s_phase) == PREPARING) vTaskDelay(poll_ticks());
    // Cancellation can race this check. Business callbacks also check running()
    // at their own safe boundaries; this is not forced interruption of IO.
    if (atomic_load(&s_phase) == RUNNING) {
        atomic_store(&worker->executing, true);
        worker->callback(worker->argument);
        atomic_store(&worker->executing, false);
    }
    atomic_fetch_sub(&s_pending, 1);
    vTaskDelete(NULL); // Never delete another task or its in-flight resources.
}

bool read_pico_background_preparing(void) { return atomic_load(&s_phase) == PREPARING; }
bool read_pico_background_running(void) { return atomic_load(&s_phase) == RUNNING; }
void read_pico_background_cancel(void) { atomic_store(&s_phase, CANCELLED); }

esp_err_t read_pico_background_add(read_pico_background_role_t role, TaskFunction_t callback,
                                   void *argument, uint32_t stack_bytes, UBaseType_t priority)
{
    if ((unsigned) role >= READ_PICO_BACKGROUND_COUNT || !callback || !stack_bytes)
        return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    uint32_t bit = READ_PICO_BACKGROUND_BIT(role);
    if (read_pico_background_preparing() && !(s_created & bit)) {
        worker_t *worker = &s_workers[role];
        worker->callback = callback;
        worker->argument = argument;
        atomic_fetch_add(&s_pending, 1);
        if (xTaskCreate(worker_entry, s_names[role], stack_bytes, worker, priority, NULL) != pdPASS) {
            atomic_fetch_sub(&s_pending, 1);
            read_pico_background_cancel();
            result = ESP_ERR_NO_MEM;
        } else {
            s_created |= bit;
            result = read_pico_background_preparing() ? ESP_OK : ESP_ERR_INVALID_STATE;
        }
    }
    atomic_flag_clear(&s_control);
    return result;
}

esp_err_t read_pico_background_activate(uint32_t required_roles, esp_err_t (*publish)(void))
{
    uint32_t valid = (UINT32_C(1) << READ_PICO_BACKGROUND_COUNT) - 1;
    if (!required_roles || (required_roles & ~valid) || !publish) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (read_pico_background_preparing() && (s_created & required_roles) == required_roles) {
        result = publish();
        int expected = PREPARING;
        if (result != ESP_OK) read_pico_background_cancel();
        else if (!atomic_compare_exchange_strong(&s_phase, &expected, RUNNING))
            result = ESP_ERR_INVALID_STATE;
    }
    atomic_flag_clear(&s_control);
    return result;
}

esp_err_t read_pico_background_cancel_and_wait(TickType_t timeout_ticks)
{
    if (timeout_ticks > pdMS_TO_TICKS(5000)) return ESP_ERR_INVALID_ARG;
    read_pico_background_cancel(); // Deny further work even if a create/publish is busy.
    if (atomic_flag_test_and_set(&s_control)) return ESP_ERR_INVALID_STATE;
    TaskHandle_t caller = xTaskGetCurrentTaskHandle();
    unsigned self = 0;
    for (unsigned i = 0; i < READ_PICO_BACKGROUND_COUNT; ++i) {
        if (caller && atomic_load(&s_workers[i].caller) == caller &&
            atomic_load(&s_workers[i].executing)) self = 1;
    }
    TickType_t started = xTaskGetTickCount();
    esp_err_t result = ESP_ERR_TIMEOUT;
    while (true) {
        if (atomic_load(&s_pending) <= self) { result = ESP_OK; break; }
        TickType_t elapsed = xTaskGetTickCount() - started;
        if (elapsed >= timeout_ticks) break;
        TickType_t remaining = timeout_ticks - elapsed;
        vTaskDelay(remaining < poll_ticks() ? remaining : poll_ticks());
    }
    atomic_flag_clear(&s_control);
    return result;
}
