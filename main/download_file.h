#pragma once
#include "upload_file.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum { DOWNLOAD_FAILED, DOWNLOAD_COMPLETE, DOWNLOAD_NOT_MODIFIED } download_result_t;
// The SDK may ignore event callback errors. Failure is latched in the owner,
// so a later successful HTTP perform cannot claim incomplete/failed writes.
bool download_file_write(upload_file_t *owner, int status, const void *data,
                         int length, uint64_t elapsed_ms);
download_result_t download_file_finish(upload_file_t *owner, bool transport_ok,
                                      int status, bool complete, int64_t content_length);
// Conservative HTTP origin comparison. Unknown/userinfo URLs are never trusted
// to receive the configured image-source credentials.
bool download_same_origin(const char *a, const char *b);
bool download_redirect_same_origin(const char *origin, const char *location);
#ifdef __cplusplus
}
#endif
