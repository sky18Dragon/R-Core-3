#pragma once
#include "esp_err.h"
#define unlikely(value) (value)
#define ESP_RETURN_ON_FALSE(condition, error, tag, ...) do { if (!(condition)) return error; } while (0)
