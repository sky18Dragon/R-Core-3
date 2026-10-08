#include "config_read.h"
#include <string.h>

int config_read_result(config_read_guard_t *guard, int result)
{
    if (result && result != guard->missing && !guard->error) guard->error = result;
    return result;
}
int config_read_string(config_read_guard_t *guard, int result, const char *value,
                       size_t capacity, size_t reported)
{
    if (!result && (!value || !reported || reported > capacity ||
                    memchr(value, 0, reported) != value + reported - 1)) result = guard->malformed;
    return config_read_result(guard, result);
}
int config_read_integer(config_read_guard_t *guard, int result, int64_t value,
                        int64_t minimum, int64_t maximum)
{
    if (!result && (value < minimum || value > maximum)) result = guard->malformed;
    return config_read_result(guard, result);
}
