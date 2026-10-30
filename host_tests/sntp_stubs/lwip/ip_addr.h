#pragma once
#include <stdint.h>
typedef struct { uint32_t value; } ip_addr_t;
#define ip_addr_isany(a) ((a)->value == 0)
#define ip_addr_ismulticast(a) (((a)->value & 0xf0000000) == 0xe0000000)
#define ip_addr_set_zero(a) ((a)->value = 0)
