#include "hotspot_policy.h"
#include <string.h>

static uint32_t minutes_to_ms(uint32_t minutes) { return minutes * 60u * 1000u; }

void hotspot_policy_init(hotspot_policy_t *policy, uint32_t idle_minutes)
{
    memset(policy, 0, sizeof(*policy));
    policy->idle_limit_ms = minutes_to_ms(idle_minutes);
}

void hotspot_policy_set_idle_minutes(hotspot_policy_t *policy, uint32_t idle_minutes, int64_t now_ms)
{
    policy->idle_limit_ms = minutes_to_ms(idle_minutes);
    policy->idle_since_ms = now_ms;  // a new limit counts from now, not from the last client
}

bool hotspot_policy_wants_on(const hotspot_policy_t *policy)
{
    return policy->pending ? policy->want_on
                           : policy->state == HOTSPOT_ON || policy->state == HOTSPOT_STARTING;
}

bool hotspot_policy_owns_radio(const hotspot_policy_t *policy)
{
    return policy->state == HOTSPOT_STARTING || policy->state == HOTSPOT_ON ||
           policy->state == HOTSPOT_STOPPING || policy->stop_unconfirmed ||
           (policy->pending && policy->want_on);
}

bool hotspot_policy_request(hotspot_policy_t *policy, bool on, int64_t now_ms)
{
    if (on && policy->stop_unconfirmed) return false;
    // An unconfirmed stop is "off" already, yet an off request must retry it.
    bool retry_stop = !on && policy->stop_unconfirmed;
    if (on == hotspot_policy_wants_on(policy) && !retry_stop) return true;
    policy->pending = true;
    policy->want_on = on;
    policy->request_at_ms = now_ms + HOTSPOT_REQUEST_DELAY_MS;
    if (!on) policy->stop_reason = HOTSPOT_REASON_USER;
    return true;
}

hotspot_step_t hotspot_policy_step(hotspot_policy_t *policy, int64_t now_ms, unsigned clients,
                                   bool busy)
{
    if (policy->state == HOTSPOT_STARTING || policy->state == HOTSPOT_STOPPING)
        return HOTSPOT_STEP_NONE;  // the owner reports the result first
    if (policy->state == HOTSPOT_ON && (clients || busy)) policy->idle_since_ms = now_ms;
    if (policy->pending && now_ms >= policy->request_at_ms) {
        if (policy->want_on && policy->state != HOTSPOT_ON) {
            policy->pending = false;
            policy->state = HOTSPOT_STARTING;
            return HOTSPOT_STEP_START;
        }
        if (!policy->want_on && (policy->state == HOTSPOT_ON || policy->stop_unconfirmed)) {
            if (busy) return HOTSPOT_STEP_NONE;  // never cut an upload; retried next step
            policy->pending = false;
            policy->state = HOTSPOT_STOPPING;
            return HOTSPOT_STEP_STOP;
        }
        policy->pending = false;  // already in the wished state
    }
    if (policy->state == HOTSPOT_ON && !policy->pending && policy->idle_limit_ms &&
        now_ms - policy->idle_since_ms >= (int64_t) policy->idle_limit_ms) {
        policy->state = HOTSPOT_STOPPING;
        policy->stop_reason = HOTSPOT_REASON_IDLE;
        return HOTSPOT_STEP_STOP;
    }
    return HOTSPOT_STEP_NONE;
}

void hotspot_policy_started(hotspot_policy_t *policy, bool ok, int64_t now_ms)
{
    if (policy->state != HOTSPOT_STARTING) return;
    policy->state = ok ? HOTSPOT_ON : HOTSPOT_FAILED;
    policy->idle_since_ms = now_ms;
    if (!ok && policy->pending && policy->want_on) policy->pending = false;
}

void hotspot_policy_stopped(hotspot_policy_t *policy, bool ok)
{
    if (policy->state != HOTSPOT_STOPPING) return;
    policy->state = ok ? HOTSPOT_OFF : HOTSPOT_FAILED;
    policy->stop_unconfirmed = !ok;
    policy->last_stop_reason = policy->stop_reason;
    policy->stop_reason = HOTSPOT_REASON_NONE;
}

int64_t hotspot_policy_remaining_ms(const hotspot_policy_t *policy, int64_t now_ms)
{
    if (policy->state != HOTSPOT_ON || !policy->idle_limit_ms) return -1;
    int64_t left = (int64_t) policy->idle_limit_ms - (now_ms - policy->idle_since_ms);
    return left > 0 ? left : 0;
}

const char *hotspot_state_name(hotspot_state_t state)
{
    switch (state) {
    case HOTSPOT_OFF: return "off";
    case HOTSPOT_STARTING: return "starting";
    case HOTSPOT_ON: return "on";
    case HOTSPOT_STOPPING: return "stopping";
    case HOTSPOT_FAILED: return "failed";
    }
    return "unknown";
}

const char *hotspot_reason_name(hotspot_reason_t reason)
{
    switch (reason) {
    case HOTSPOT_REASON_NONE: return "none";
    case HOTSPOT_REASON_USER: return "user";
    case HOTSPOT_REASON_IDLE: return "idle";
    }
    return "unknown";
}

bool hotspot_idle_minutes_valid(uint32_t minutes, bool battery)
{
    return (minutes == 0 && !battery) || minutes == 10 || minutes == 30 || minutes == 60;
}

bool hotspot_password_generate(const uint8_t *random, size_t count, char *out)
{
    static const char alphabet[] = HOTSPOT_PASSWORD_ALPHABET;
    const unsigned size = sizeof(alphabet) - 1;
    const unsigned limit = 256 - 256 % size;  // reject the biased tail
    if (!random || !out) return false;
    char next[HOTSPOT_PASSWORD_LEN + 1];
    size_t used = 0;
    for (size_t i = 0; i < count && used < HOTSPOT_PASSWORD_LEN; ++i)
        if (random[i] < limit) next[used++] = alphabet[random[i] % size];
    if (used < HOTSPOT_PASSWORD_LEN) return false;
    next[HOTSPOT_PASSWORD_LEN] = 0;
    memcpy(out, next, sizeof(next));
    return true;
}

bool hotspot_password_valid(const char *password)
{
    if (!password) return false;
    size_t length = strnlen(password, HOTSPOT_PASSWORD_MAX + 1);
    if (length < 8 || length > HOTSPOT_PASSWORD_MAX) return false;
    for (size_t i = 0; i < length; ++i)
        if (password[i] < 0x20 || password[i] > 0x7e) return false;
    return true;
}

bool hotspot_ssid_valid(const char *ssid)
{
    if (!ssid) return false;
    size_t length = strnlen(ssid, HOTSPOT_SSID_MAX + 1);
    if (!length || length > HOTSPOT_SSID_MAX) return false;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char) ssid[i];
        if (c < 0x20 || c == 0x7f) return false;
    }
    return true;
}

static bool append_escaped(char *out, size_t capacity, size_t *used, const char *text)
{
    for (; *text; ++text) {
        bool special = strchr("\\;,:\"", *text) != NULL;
        if (*used + special + 1 >= capacity) return false;
        if (special) out[(*used)++] = '\\';
        out[(*used)++] = *text;
    }
    return true;
}

static bool append_text(char *out, size_t capacity, size_t *used, const char *text)
{
    size_t length = strlen(text);
    if (*used + length >= capacity) return false;
    memcpy(out + *used, text, length);
    *used += length;
    return true;
}

bool hotspot_wifi_qr(const char *ssid, const char *password, char *out, size_t capacity)
{
    if (!hotspot_ssid_valid(ssid) || !hotspot_password_valid(password) || !out || !capacity)
        return false;
    char next[2 * (HOTSPOT_SSID_MAX + HOTSPOT_PASSWORD_MAX) + 32];
    size_t used = 0;
    if (!append_text(next, sizeof(next), &used, "WIFI:T:WPA;S:") ||
        !append_escaped(next, sizeof(next), &used, ssid) ||
        !append_text(next, sizeof(next), &used, ";P:") ||
        !append_escaped(next, sizeof(next), &used, password) ||
        !append_text(next, sizeof(next), &used, ";;"))
        return false;
    if (used + 1 > capacity) return false;
    memcpy(out, next, used);
    out[used] = 0;
    return true;
}

bool hotspot_clock_valid(int64_t unix_seconds)
{
    return unix_seconds >= HOTSPOT_CLOCK_MIN && unix_seconds < HOTSPOT_CLOCK_MAX;
}
