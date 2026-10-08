#pragma once
#include <sys/time.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { SNTP_SYNC_MODE_IMMED, SNTP_SYNC_MODE_SMOOTH } sntp_sync_mode_t;
typedef enum { SNTP_SYNC_STATUS_RESET, SNTP_SYNC_STATUS_COMPLETED } sntp_sync_status_t;
typedef void (*sntp_sync_time_cb_t)(struct timeval *);
void sntp_set_sync_mode(sntp_sync_mode_t);
void sntp_set_sync_status(sntp_sync_status_t);
void sntp_set_time_sync_notification_cb(sntp_sync_time_cb_t);
void sntp_sync_time(struct timeval *);
#ifdef __cplusplus
}
#endif
