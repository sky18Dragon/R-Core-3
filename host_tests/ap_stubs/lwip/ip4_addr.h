#pragma once
#include "wifi_sdk.h"
#define IP4_ADDR(ip,a,b,c,d) ((ip)->addr = ((uint32_t)(a) | (uint32_t)(b)<<8 | \
                                        (uint32_t)(c)<<16 | (uint32_t)(d)<<24))
