#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// A singleton response field is consumed even when its value is invalid.
// A later occurrence must never repair/replace an earlier rejected field.
static inline bool text_buffer_claim_once(bool *seen)
{
    if (!seen || *seen) return false;
    *seen = true;
    return true;
}

// Protocol fields must fit exactly, unlike diagnostic text. Failure does not
// publish a truncated prefix or alter the caller's previous value.
static inline bool text_buffer_copy_exact(char *out, size_t capacity, const char *value)
{
    if (!out || !capacity || !value) return false;
    size_t length = strnlen(value, capacity);
    if (length == capacity) return false;
    memcpy(out, value, length + 1);
    return true;
}

// Copy a diagnostic into a caller-owned buffer. Source is a valid,
// non-overlapping NUL-terminated string; NULL source means empty text.
// Zero capacity and NULL destination are valid no-output requests.
static inline void text_buffer_message(char *out, size_t capacity, const char *message)
{
    if (!out || !capacity) return;
    size_t length = message ? strnlen(message, capacity - 1) : 0;
    if (length) memcpy(out, message, length);
    out[length] = '\0';
}
