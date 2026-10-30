#pragma once
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { char host[256]; unsigned port; bool tls; } http_endpoint_t;
// Absolute HTTP(S), no embedded credentials. Uses the SDK HTTP URL parser.
// Input must be NUL terminated within 2048 bytes; failure leaves output intact.
bool http_endpoint_parse(const char *url, http_endpoint_t *out);
// Same policy when the caller needs only authorization, not endpoint fields.
bool http_endpoint_valid(const char *url);
// Validate a redirect reference without resolving or rewriting it. Absolute
// and network-path authorities must satisfy the same no-userinfo policy.
// SDK resolution/downgrade rules still apply after this check.
bool http_redirect_reference_valid(const char *reference);
// Host authority for an absolute HTTP(S) URL; only the scheme's own default
// port is omitted. IPv6 is bracketed. Failure leaves output unchanged.
bool http_endpoint_host_header(const char *url, char *out, size_t capacity);
#ifdef __cplusplus
}
#endif
