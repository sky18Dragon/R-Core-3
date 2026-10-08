#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// A current AP save must finish; old NVS credentials cannot satisfy this gate.
uint32_t provisioning_session_begin(void);
uint32_t provisioning_session_ticket(void);
bool provisioning_session_commit(uint32_t ticket);
bool provisioning_session_complete(void);
// Invalidate saves when the owning AP stops/fails; no persistence or rollback.
void provisioning_session_cancel(void);
#ifdef __cplusplus
}
#endif
