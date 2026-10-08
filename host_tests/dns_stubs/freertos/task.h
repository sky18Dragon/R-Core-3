#pragma once
#include "freertos/FreeRTOS.h"
#include <stdint.h>
typedef void (*TaskFunction_t)(void *);
typedef void *TaskHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
BaseType_t xTaskCreate(TaskFunction_t, const char *, uint32_t, void *, unsigned, TaskHandle_t *);
void vTaskDelay(TickType_t);
void vTaskDelete(TaskHandle_t);
#ifdef __cplusplus
}
#endif
