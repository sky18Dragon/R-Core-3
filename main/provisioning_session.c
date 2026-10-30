#include "provisioning_session.h"
#include <stdatomic.h>
static atomic_uint session;
uint32_t provisioning_session_begin(void)
{
    unsigned old = atomic_load(&session);
    for (;;) {
        if ((old >> 2) >= 0x3fffffffU) return 0; // Never reuse a ticket after wrap.
        unsigned next = ((old >> 2) + 1) << 2;
        if (atomic_compare_exchange_weak(&session, &old, next)) return next;
    }
}
uint32_t provisioning_session_ticket(void)
{
    unsigned state = atomic_load(&session);
    return (state & 3U) ? 0 : state; // Completed/cancelled sessions cannot start saves.
}
bool provisioning_session_commit(uint32_t ticket)
{
    if (!ticket || (ticket & 3)) return false;
    unsigned expected = ticket;
    return atomic_compare_exchange_strong(&session, &expected, ticket | 1U);
}
bool provisioning_session_complete(void) { return (atomic_load(&session) & 3U) == 1U; }
void provisioning_session_cancel(void)
{
    unsigned old = atomic_load(&session);
    // Keep the generation: cancellation must not make an old ticket reusable.
    while (old && (old & 3U) != 2U &&
           !atomic_compare_exchange_weak(&session, &old, (old & ~3U) | 2U)) {}
}
