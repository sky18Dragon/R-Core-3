#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "wifi_network.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t ticket;
    char ssid[33], password[65], name[64];
    wifi_network_t network;
} provisioning_apply_request_t;
typedef struct {
    bool (*current)(void *, uint32_t ticket);
    int (*test)(void *, const provisioning_apply_request_t *);
    int (*save)(void *, const provisioning_apply_request_t *);
    int (*respond)(void *, const provisioning_apply_request_t *);
    bool (*complete)(void *, uint32_t ticket);
} provisioning_apply_ops_t;
typedef enum {
    PROVISION_APPLY_DONE, PROVISION_APPLY_INVALID, PROVISION_APPLY_STALE, PROVISION_APPLY_SAVED_STALE,
    PROVISION_APPLY_TEST, PROVISION_APPLY_SAVE, PROVISION_APPLY_RESPONSE,
    PROVISION_APPLY_COMPLETE
} provisioning_apply_stage_t;
typedef struct { provisioning_apply_stage_t stage; int error; } provisioning_apply_result_t;
// Synchronous existing handler sequencing. Copy the validated request so a
// candidate test cannot change the group later saved. No IO or state ownership.
// A failure never invokes later stages; post-save failures do not promise rollback.
provisioning_apply_result_t provisioning_apply_run(const provisioning_apply_request_t *,
                                                   const provisioning_apply_ops_t *, void *context);
#ifdef __cplusplus
}
#endif
