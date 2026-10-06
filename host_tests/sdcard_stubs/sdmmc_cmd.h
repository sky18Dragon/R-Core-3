#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int marker;
} sdmmc_card_t;

void sdmmc_card_print_info(FILE *stream, const sdmmc_card_t *card);

#ifdef __cplusplus
}
#endif
