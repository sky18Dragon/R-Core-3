#ifndef REQUEST_CONFIG_H
#define REQUEST_CONFIG_H
#include "esp_err.h"

typedef struct {
    char revision[24];
    char *processing;
    char *palette;
} request_config_t;

// Read Pico: capture the three configuration headers under participating
// application control. Output is unchanged on error. Success owns both JSON
// strings; release a previous successful snapshot before capturing another.
esp_err_t request_config_capture(request_config_t *out);
void request_config_release(request_config_t *snapshot);

typedef struct {
    void *context;
    esp_err_t (*remove)(void *context, const char *key);
    esp_err_t (*get)(void *context, const char *key, const char **value);
    esp_err_t (*set)(void *context, const char *key, const char *value);
} request_header_ops_t;
typedef struct {
    void *context;
    esp_err_t (*prepare_url)(void *context);
    esp_err_t (*confirm_host)(void *context);
    esp_err_t (*schedule)(void *context);
} request_redirect_ops_t;
// Preparation callbacks must not schedule/send requests. First error stops;
// only a fully prepared target may reach schedule. No rollback is promised.
esp_err_t request_redirect_schedule_checked(const request_redirect_ops_t *ops);
// Confirm absence after removal. Only remove/get callbacks are required.
esp_err_t request_header_remove_checked(const request_header_ops_t *ops, const char *key);
// Remove Authorization then the optional custom credential header (NULL/empty
// means none). Validate first, stop at the first error, never restore secrets.
esp_err_t request_headers_strip_credentials(const request_header_ops_t *ops,
                                            const char *custom_key);
// Nonempty ASCII token key, value without CR/LF/DEL or controls except HTAB.
// Confirm removal and exact stored value; SDK trimming is not silently accepted.
// The caller owns valid NUL-terminated key/value for the duration of this call.
esp_err_t request_header_put_checked(const request_header_ops_t *ops,
                                     const char *key, const char *value);
// Replace these three headers, confirming absence before set and exact owned
// value afterwards. Failure may leave partial headers: discard the client,
// never perform the request. Callback value is borrowed only until next call.
esp_err_t request_config_set_headers(const request_config_t *snapshot,
                                     const request_header_ops_t *ops);
#endif
