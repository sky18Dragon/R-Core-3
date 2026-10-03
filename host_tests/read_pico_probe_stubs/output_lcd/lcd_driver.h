#pragma once
#include <stdbool.h>
#include <stdint.h>
// Host boundary only; actual locked vendor signatures checked by IDF build.
typedef bool (*line_cb_func_t)(void *, uint8_t *);
typedef void (*frame_done_func_t)(void *);
