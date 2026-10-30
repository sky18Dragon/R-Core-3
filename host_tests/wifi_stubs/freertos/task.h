#pragma once
#include "freertos/FreeRTOS.h"
typedef void *TaskHandle_t;
#ifdef __cplusplus
extern "C" {
#endif
void vTaskDelay(TickType_t);
TaskHandle_t xTaskGetCurrentTaskHandle(void);
#ifdef __cplusplus
}
#endif
