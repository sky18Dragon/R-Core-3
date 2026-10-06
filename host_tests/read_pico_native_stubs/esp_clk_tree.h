#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "soc/clk_tree_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
} esp_clk_tree_src_freq_precision_t;

esp_err_t esp_clk_tree_src_get_freq_hz(soc_module_clk_t source,
                                       esp_clk_tree_src_freq_precision_t precision,
                                       uint32_t *frequency_hz);

#ifdef __cplusplus
}
#endif
