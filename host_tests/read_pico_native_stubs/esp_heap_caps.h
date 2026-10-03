#pragma once
#include <stddef.h>
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_INTERNAL 2
#define MALLOC_CAP_8BIT 4
#ifdef __cplusplus
extern "C" {
#endif
void *heap_caps_aligned_alloc(size_t, size_t, unsigned);
void *heap_caps_calloc(size_t, size_t, unsigned);
void heap_caps_free(void *);
size_t heap_caps_get_largest_free_block(unsigned);
#ifdef __cplusplus
}
#endif
