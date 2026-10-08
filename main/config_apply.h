#ifndef CONFIG_APPLY_H
#define CONFIG_APPLY_H
#include <stdbool.h>
#include "esp_err.h"
// Zero-wait application control shared by the participating configuration
// adapters. This is not a lock for every SDK consumer or direct setter.
esp_err_t config_apply_run(esp_err_t (*apply)(void *context), void *context);
typedef struct {
    bool entered;
    bool applied;
    bool finished;
} config_apply_result_t;
// Keep apply and revision/finish under the same control. Finish is skipped on
// apply failure. Results describe callback returns, not rollback or durability
// of every internal side effect. Busy does not call either callback.
esp_err_t config_apply_transaction(esp_err_t (*apply)(void *context),
                                   esp_err_t (*finish)(void *context), void *context,
                                   config_apply_result_t *result);
#endif
