#include "config_apply.h"
#include <stdatomic.h>
#include <stddef.h>
static atomic_flag applying = ATOMIC_FLAG_INIT;
esp_err_t config_apply_run(esp_err_t (*apply)(void *context), void *context)
{
    if (!apply) return ESP_ERR_INVALID_ARG;
    if (atomic_flag_test_and_set(&applying)) return ESP_ERR_INVALID_STATE;
    esp_err_t err = apply(context);
    atomic_flag_clear(&applying);
    return err;
}
typedef struct {
    esp_err_t (*apply)(void *);
    esp_err_t (*finish)(void *);
    void *context;
    config_apply_result_t *result;
} transaction_t;
static esp_err_t run_transaction(void *context)
{
    transaction_t *request = context;
    request->result->entered = true;
    esp_err_t err = request->apply(request->context);
    if (err != ESP_OK) return err;
    request->result->applied = true;
    err = request->finish(request->context);
    request->result->finished = err == ESP_OK;
    return err;
}
esp_err_t config_apply_transaction(esp_err_t (*apply)(void *),
                                   esp_err_t (*finish)(void *), void *context,
                                   config_apply_result_t *result)
{
    if (!apply || !finish || !result) return ESP_ERR_INVALID_ARG;
    *result = (config_apply_result_t){0};
    transaction_t request = {apply, finish, context, result};
    return config_apply_run(run_transaction, &request);
}
