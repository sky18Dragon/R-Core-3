#pragma once
#include <stdint.h>

#include "freertos/FreeRTOS.h"
typedef void *SemaphoreHandle_t;
typedef struct {
    uintptr_t unused;
} StaticSemaphore_t;
#ifdef __cplusplus
extern "C" {
#endif
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *);
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t);
BaseType_t xSemaphoreGive(SemaphoreHandle_t);
#ifdef __cplusplus
}
#endif
