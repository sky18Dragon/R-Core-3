// Host-test stub for esp_heap_caps.h — caps are ignored; everything is plain
// malloc so leak checkers still work.
#pragma once

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_8BIT 0
#define MALLOC_CAP_DMA 0
#define MALLOC_CAP_DEFAULT 0

#ifdef READ_PICO_JPEG_PIPELINE_TESTS
extern size_t test_heap_max_request;
extern size_t test_heap_request_limit;
#endif

static inline void *heap_caps_malloc(size_t size, unsigned caps)
{
    (void) caps;
#ifdef READ_PICO_JPEG_PIPELINE_TESTS
    if (size > test_heap_max_request)
        test_heap_max_request = size;
    if (size > test_heap_request_limit)
        return NULL;
#endif
    return malloc(size);
}

static inline void *heap_caps_calloc(size_t n, size_t size, unsigned caps)
{
    (void) caps;
#ifdef READ_PICO_JPEG_PIPELINE_TESTS
    if (size && n > SIZE_MAX / size)
        return NULL;
    if (n * size > test_heap_max_request)
        test_heap_max_request = n * size;
    if (n * size > test_heap_request_limit)
        return NULL;
#endif
    return calloc(n, size);
}

static inline void heap_caps_free(void *ptr)
{
    free(ptr);
}

#ifdef __cplusplus
}
#endif
