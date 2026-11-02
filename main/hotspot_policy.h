#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Portable policy for the photo-transfer hotspot (docs/PRD-HOTSPOT-SETTINGS.md).
// One owner task calls hotspot_policy_step() periodically and performs the
// returned radio step, then reports the result. No SDK, NVS or display here.
#define HOTSPOT_PASSWORD_LEN 8
// Generated passwords avoid look-alikes (0/O, 1/I/L) so they can be typed from the screen.
#define HOTSPOT_PASSWORD_ALPHABET "ABCDEFGHJKMNPQRSTUVWXYZ23456789"
#define HOTSPOT_PASSWORD_MAX 63
#define HOTSPOT_SSID_MAX 32
// Requests are applied after this delay so the HTTP response that asked for the
// switch reaches the phone before the radio changes.
#define HOTSPOT_REQUEST_DELAY_MS 1500
#define HOTSPOT_IDLE_DEFAULT_MIN 10
// Smallest unix time accepted from a phone (2024-01-01) and largest (2100-01-01).
#define HOTSPOT_CLOCK_MIN 1704067200LL
#define HOTSPOT_CLOCK_MAX 4102444800LL

typedef enum {
    HOTSPOT_OFF,
    HOTSPOT_STARTING,
    HOTSPOT_ON,
    HOTSPOT_STOPPING,
    HOTSPOT_FAILED,  // last start or stop was not confirmed
} hotspot_state_t;

typedef enum { HOTSPOT_STEP_NONE, HOTSPOT_STEP_START, HOTSPOT_STEP_STOP } hotspot_step_t;

typedef enum {
    HOTSPOT_REASON_NONE,
    HOTSPOT_REASON_USER,  // explicit request (screen key or web page)
    HOTSPOT_REASON_IDLE,  // no client and no upload for the idle limit
} hotspot_reason_t;

typedef struct {
    hotspot_state_t state;
    bool pending, want_on;
    bool stop_unconfirmed;    // radio exit failed: only a stop retry is allowed
    int64_t request_at_ms;    // when the pending request becomes due
    int64_t idle_since_ms;    // last time a client or upload was seen
    uint32_t idle_limit_ms;   // 0 = never close automatically
    hotspot_reason_t stop_reason, last_stop_reason;
} hotspot_policy_t;

void hotspot_policy_init(hotspot_policy_t *policy, uint32_t idle_minutes);
void hotspot_policy_set_idle_minutes(hotspot_policy_t *policy, uint32_t idle_minutes, int64_t now_ms);
// Records the latest wish; returns false when it cannot be honoured (turning on
// while a failed stop is unconfirmed). Repeating the current state is a no-op.
bool hotspot_policy_request(hotspot_policy_t *policy, bool on, int64_t now_ms);
bool hotspot_policy_wants_on(const hotspot_policy_t *policy);
// The hotspot holds, is changing or is about to take the radio (or its exit
// is unconfirmed): station recovery must not act. A failed start does not count.
bool hotspot_policy_owns_radio(const hotspot_policy_t *policy);
// clients: associated stations; busy: an upload or file operation is running
// (a stop then waits, and the idle clock does not run).
hotspot_step_t hotspot_policy_step(hotspot_policy_t *policy, int64_t now_ms, unsigned clients,
                                   bool busy);
void hotspot_policy_started(hotspot_policy_t *policy, bool ok, int64_t now_ms);
void hotspot_policy_stopped(hotspot_policy_t *policy, bool ok);
// Milliseconds until an automatic close, or -1 when none is scheduled.
int64_t hotspot_policy_remaining_ms(const hotspot_policy_t *policy, int64_t now_ms);
const char *hotspot_state_name(hotspot_state_t state);
const char *hotspot_reason_name(hotspot_reason_t reason);

// Valid automatic-close settings: 0 (never), 10, 30 or 60 minutes. Battery
// power never allows 0, so an unattended hotspot cannot drain the cell.
bool hotspot_idle_minutes_valid(uint32_t minutes, bool battery);
// Fills out[HOTSPOT_PASSWORD_LEN + 1] from random bytes by rejection sampling
// (unbiased). Returns false, leaving out unchanged, if the bytes run out.
bool hotspot_password_generate(const uint8_t *random, size_t count, char *out);
// WPA2-PSK passphrase: 8..63 printable ASCII characters.
bool hotspot_password_valid(const char *password);
// 1..32 bytes without control characters.
bool hotspot_ssid_valid(const char *ssid);
// "WIFI:T:WPA;S:<ssid>;P:<password>;;" with \ ; , : " escaped, as read by
// phone cameras. Returns false, leaving out unchanged, if it does not fit.
bool hotspot_wifi_qr(const char *ssid, const char *password, char *out, size_t capacity);
bool hotspot_clock_valid(int64_t unix_seconds);

// Retrying the saved home Wi-Fi while a recovery hotspot runs (the router was
// off or out of range at boot). An attempt never runs while a phone is joined
// or an upload runs; after they leave it waits at least the first delay.
#define HOTSPOT_RETRY_FIRST_MS 30000
#define HOTSPOT_RETRY_MAX_MS 600000

typedef struct {
    bool armed;
    int64_t due_ms;
    uint32_t backoff_ms;  // delay after the next failure
    uint32_t attempts;
} hotspot_retry_t;

void hotspot_retry_start(hotspot_retry_t *retry, int64_t now_ms);
void hotspot_retry_stop(hotspot_retry_t *retry);
// True when an attempt should run now; the caller then reports a failure with
// hotspot_retry_failed() (success ends the hotspot, so needs no call).
bool hotspot_retry_step(hotspot_retry_t *retry, int64_t now_ms, unsigned clients, bool busy);
void hotspot_retry_failed(hotspot_retry_t *retry, int64_t now_ms);
// Milliseconds until the next attempt, or -1 when none is armed.
int64_t hotspot_retry_remaining_ms(const hotspot_retry_t *retry, int64_t now_ms);
#ifdef __cplusplus
}
#endif
