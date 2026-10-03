#pragma once
#include <stdint.h>

#include "freertos/FreeRTOS.h"

typedef void *SemaphoreHandle_t;
typedef struct {
    uintptr_t unused;
} StaticSemaphore_t;
typedef void *TaskHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
SemaphoreHandle_t xSemaphoreCreateRecursiveMutexStatic(StaticSemaphore_t *);
int xSemaphoreTakeRecursive(SemaphoreHandle_t, TickType_t);
int xSemaphoreGiveRecursive(SemaphoreHandle_t);
TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
#ifdef __cplusplus
}
#endif
