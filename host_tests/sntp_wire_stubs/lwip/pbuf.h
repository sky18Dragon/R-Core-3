#pragma once
#include "lwip/opt.h"
struct pbuf { void *payload; u16_t tot_len; };
#define PBUF_TRANSPORT 0
#define PBUF_RAM 0
#ifdef __cplusplus
extern "C" {
#endif
struct pbuf *pbuf_alloc(int, u16_t, int);
u8_t pbuf_free(struct pbuf *);
u8_t pbuf_get_at(const struct pbuf *, u16_t);
u16_t pbuf_copy_partial(const struct pbuf *, void *, u16_t, u16_t);
#ifdef __cplusplus
}
#endif
