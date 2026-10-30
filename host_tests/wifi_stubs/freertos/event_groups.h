#pragma once
#include "freertos/FreeRTOS.h"
typedef void *EventGroupHandle_t;
typedef uint32_t EventBits_t;
#ifdef __cplusplus
extern "C" {
#endif
EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupSetBits(EventGroupHandle_t, EventBits_t);
EventBits_t xEventGroupClearBits(EventGroupHandle_t, EventBits_t);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t, EventBits_t, BaseType_t, BaseType_t, TickType_t);
#ifdef __cplusplus
}
#endif
