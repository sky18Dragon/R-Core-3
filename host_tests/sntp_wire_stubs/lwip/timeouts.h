#pragma once
#include "lwip/opt.h"
typedef void (*sys_timeout_handler)(void *);
#ifdef __cplusplus
extern "C" {
#endif
void sys_timeout(u32_t, sys_timeout_handler, void *);
void sys_untimeout(sys_timeout_handler, void *);
#ifdef __cplusplus
}
#endif
