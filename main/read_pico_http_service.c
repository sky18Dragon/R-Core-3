#include "read_pico_http_service.h"
#include <stdatomic.h>
#include <stddef.h>

enum { SERVICE_IDLE, SERVICE_RESERVED, SERVICE_STARTING, SERVICE_ACTIVE,
       SERVICE_STOPPING, SERVICE_FAILED, SERVICE_UNCERTAIN };
static atomic_flag s_control = ATOMIC_FLAG_INIT;
static atomic_int s_role = READ_PICO_HTTP_NONE;
static atomic_int s_state = SERVICE_IDLE;
static _Atomic(httpd_handle_t) s_handle;
static atomic_bool s_ready;

static bool valid_role(read_pico_http_role_t role)
{
    return role == READ_PICO_HTTP_FRAME || role == READ_PICO_HTTP_PROVISION;
}
static bool enter(void) { return !atomic_flag_test_and_set(&s_control); }
static esp_err_t leave(esp_err_t result) { atomic_flag_clear(&s_control); return result; }

esp_err_t read_pico_http_claim(read_pico_http_role_t role)
{
    if (!valid_role(role)) return ESP_ERR_INVALID_ARG;
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (s_state != SERVICE_IDLE) return leave(ESP_ERR_INVALID_STATE);
    s_role = role; s_ready = false; s_state = SERVICE_RESERVED;
    return leave(ESP_OK);
}

esp_err_t read_pico_http_start(read_pico_http_role_t role, const httpd_config_t *config,
                              esp_err_t (*routes)(httpd_handle_t))
{
    if (!valid_role(role) || !config || !routes || config->server_port != 80)
        return ESP_ERR_INVALID_ARG;
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (s_role != role || s_state != SERVICE_RESERVED || s_handle)
        return leave(ESP_ERR_INVALID_STATE);
    s_ready = false; s_state = SERVICE_STARTING;
    httpd_handle_t created = NULL;
    esp_err_t error = httpd_start(&created, config);
    if (error != ESP_OK || !created) {
        // Fixed SDK publishes only on success. Never close a guessed handle on
        // contract drift or discard a successful but unpublished server.
        s_state = created || error == ESP_OK ? SERVICE_UNCERTAIN : SERVICE_RESERVED;
        return leave(error != ESP_OK ? error : ESP_ERR_INVALID_STATE);
    }
    s_handle = created; // Exact owned handle, including a route failure.
    error = routes(created);
    s_state = error == ESP_OK ? SERVICE_ACTIVE : SERVICE_FAILED;
    return leave(error);
}

esp_err_t read_pico_http_stop(read_pico_http_role_t role)
{
    if (!valid_role(role)) return ESP_ERR_INVALID_ARG;
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (s_state == SERVICE_IDLE) return leave(ESP_OK);
    if (s_role != role) return leave(ESP_ERR_INVALID_STATE);
    s_ready = false;
    if (s_state == SERVICE_UNCERTAIN) return leave(ESP_ERR_INVALID_STATE);
    s_state = SERVICE_STOPPING;
    httpd_handle_t owned = s_handle;
    if (owned) {
        esp_err_t error = httpd_stop(owned);
        if (error != ESP_OK) { s_state = SERVICE_FAILED; return leave(error); }
        s_handle = NULL; // Fixed SDK frees only on a successful stop.
    }
    s_state = SERVICE_RESERVED;
    return leave(ESP_OK);
}

esp_err_t read_pico_http_release(read_pico_http_role_t role)
{
    if (!valid_role(role)) return ESP_ERR_INVALID_ARG;
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (s_state == SERVICE_IDLE) return leave(ESP_OK);
    if (s_role != role || s_handle || s_state != SERVICE_RESERVED)
        return leave(ESP_ERR_INVALID_STATE);
    s_ready = false; s_role = READ_PICO_HTTP_NONE; s_state = SERVICE_IDLE;
    return leave(ESP_OK);
}

esp_err_t read_pico_http_mark_ready(read_pico_http_role_t role)
{
    if (!valid_role(role)) return ESP_ERR_INVALID_ARG;
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (s_role != role || s_state != SERVICE_ACTIVE || !s_handle)
        return leave(ESP_ERR_INVALID_STATE);
    s_ready = true;
    return leave(ESP_OK);
}

bool read_pico_http_active(read_pico_http_role_t role)
{
    return valid_role(role) && s_role == role && s_state == SERVICE_ACTIVE;
}

bool read_pico_http_accepts(read_pico_http_role_t role, httpd_handle_t handle)
{
    return handle && read_pico_http_ready(role) && s_handle == handle;
}

bool read_pico_http_ready(read_pico_http_role_t role)
{
    return read_pico_http_active(role) && s_ready && s_handle;
}
