#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
// Portable read-only load validation. Only the configured missing-key result
// permits a default; type/length/IO errors remain latched until startup stops.
typedef struct { int error, missing, malformed; } config_read_guard_t;
int config_read_result(config_read_guard_t *, int result);
int config_read_string(config_read_guard_t *, int result, const char *value,
                       size_t capacity, size_t reported);
int config_read_integer(config_read_guard_t *, int result, int64_t value,
                        int64_t minimum, int64_t maximum);
#ifdef __cplusplus
}
#endif
