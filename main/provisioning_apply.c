#include "provisioning_apply.h"
#include "wifi_text.h"
#include <string.h>
provisioning_apply_result_t provisioning_apply_run(const provisioning_apply_request_t *input,
                                                   const provisioning_apply_ops_t *ops, void *ctx)
{
    if (!input || !ops || !ops->current || !ops->test || !ops->save || !ops->respond || !ops->complete)
        return (provisioning_apply_result_t){PROVISION_APPLY_INVALID, -1};
    provisioning_apply_request_t request = *input;
    if (!request.ticket || !wifi_text_credentials_valid(request.ssid, request.password) ||
        !request.name[0] || strnlen(request.name, sizeof(request.name)) >= sizeof(request.name) ||
        !wifi_network_valid(&request.network))
        return (provisioning_apply_result_t){PROVISION_APPLY_INVALID, -1};
    if (!ops->current(ctx, request.ticket))
        return (provisioning_apply_result_t){PROVISION_APPLY_STALE, -1};
    int err = ops->test(ctx, &request);
    if (err) return (provisioning_apply_result_t){PROVISION_APPLY_TEST, err};
    if (!ops->current(ctx, request.ticket))
        return (provisioning_apply_result_t){PROVISION_APPLY_STALE, -1};
    err = ops->save(ctx, &request);
    if (err) return (provisioning_apply_result_t){PROVISION_APPLY_SAVE, err};
    if (!ops->current(ctx, request.ticket))
        return (provisioning_apply_result_t){PROVISION_APPLY_SAVED_STALE, -1};
    err = ops->respond(ctx, &request);
    if (err) return (provisioning_apply_result_t){PROVISION_APPLY_RESPONSE, err};
    if (!ops->complete(ctx, request.ticket))
        return (provisioning_apply_result_t){PROVISION_APPLY_COMPLETE, -1};
    return (provisioning_apply_result_t){PROVISION_APPLY_DONE, 0};
}
