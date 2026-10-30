#ifndef PERIODIC_TASKS_H
#define PERIODIC_TASKS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

// Periodic tasks names
#define SNTP_TASK_NAME "sntp_sync"

/**
 * @brief Periodic task callback function type
 *
 * @return ESP_OK if task completed successfully, error code otherwise
 */
typedef esp_err_t (*periodic_task_callback_t)(void);

/**
 * @brief Initialize the periodic tasks manager
 *
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_init(void);

/**
 * @brief Register a periodic task
 *
 * @param task_name Unique name for the task (used as NVS key)
 * @param callback Function to call when task should run
 * @param interval_seconds How often to run the task (in seconds)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_register(const char *task_name, periodic_task_callback_t callback,
                                  uint32_t interval_seconds);

/**
 * @brief Check if any registered tasks should run and execute them
 *
 * This should be called periodically (e.g., on boot, after WiFi connects)
 *
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_check_and_run(void);

/**
 * @brief Check if a specific task should run based on its interval
 *
 * @param task_name Name of the task to check
 * @return true if task should run, false otherwise
 */
bool periodic_tasks_should_run(const char *task_name);

/**
 * @brief Update the last run time for a task to current time
 *
 * @param task_name Name of the task
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_update_last_run(const char *task_name);

/**
 * @brief Get the last run time for a task
 *
 * @param task_name Name of the task
 * @param last_run_time Pointer to store the last run time (Unix timestamp)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_get_last_run(const char *task_name, int64_t *last_run_time);

/**
 * @brief Force a task to run on next check by clearing its last run time
 *
 * @param task_name Name of the task
 * @return ESP_OK on success, error code on failure
 */
esp_err_t periodic_tasks_force_run(const char *task_name);

#if CONFIG_BOARD_DRIVER_READ_PICO
// Admission closes immediately; stop confirms owned timer exit only when no
// batch/NVS operation is active. No task is forcibly deleted or state reused.
void periodic_tasks_request_stop(void);
esp_err_t periodic_tasks_stop(void);
bool periodic_tasks_is_active(void);
// Outputs remain unchanged on errors. The bool compatibility API fails closed.
esp_err_t periodic_tasks_should_run_checked(const char *task_name, bool *should_run);
// Atomic admission/force/execute for one task; no gap in which another caller
// can consume this request and make a subsequent no-op look like its success.
esp_err_t periodic_tasks_run_now(const char *task_name);
#endif

#endif  // PERIODIC_TASKS_H
