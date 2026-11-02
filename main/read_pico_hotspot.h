#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
// Device hotspot for photo transfer and Wi-Fi setup (docs/PRD-HOTSPOT-SETTINGS.md,
// H1 + H3), always WPA2. Any task may
// request a change; the button/touch task owns the radio switch by calling
// read_pico_hotspot_poll(). While on, the home station is off (the two are
// exclusive), a WPA2 AP serves 192.168.4.1 and the frame HTTP server keeps
// serving. Off restores the screen and reconnects the saved station.
#define READ_PICO_HOTSPOT_URL "http://192.168.4.1"

typedef enum {
    READ_PICO_HOTSPOT_MANUAL,    // turned on by the user (web page or middle key)
    READ_PICO_HOTSPOT_SETUP,     // boot without saved Wi-Fi: set up or use offline
    READ_PICO_HOTSPOT_RECOVERY,  // saved Wi-Fi unreachable at boot: retried in the background
} read_pico_hotspot_mode_t;

typedef struct {
    const char *ssid;          // NULL = unchanged, "" = back to the device default
    const char *password;      // NULL = unchanged; 8..63 printable ASCII
    bool regenerate_password;  // new random password (ignored if password is set)
    int idle_minutes;          // -1 = unchanged; 0 (never), 10, 30 or 60
} read_pico_hotspot_change_t;

// Loads settings; creates and saves a random password on first use. Never
// starts the radio: the hotspot is off after every boot.
esp_err_t read_pico_hotspot_init(void);
void read_pico_hotspot_poll(int64_t now_ms);
// ESP_ERR_INVALID_STATE when turning on is refused (an earlier stop was not confirmed).
esp_err_t read_pico_hotspot_request(bool on);
esp_err_t read_pico_hotspot_toggle(void);
// Boot fallback instead of the old open setup portal: the frame keeps running
// and opens the encrypted hotspot. It does not close by itself unless the build
// may run on battery. RECOVERY retries the saved Wi-Fi while no phone is joined
// and restarts into it once it answers. A saved setup also restarts the frame.
esp_err_t read_pico_hotspot_start_fallback(read_pico_hotspot_mode_t mode);
// Middle key short press: show the connection screen again while on.
esp_err_t read_pico_hotspot_show_screen(void);
// The hotspot holds or is changing the radio: station recovery must not run.
bool read_pico_hotspot_owns_radio(void);
// The AP is up and serving (connectivity probes answer only then).
bool read_pico_hotspot_on(void);
// Changes apply from the next start; a running hotspot keeps its settings.
esp_err_t read_pico_hotspot_configure(const read_pico_hotspot_change_t *change);
// Status for /api/hotspot, including the password (the page is behind web auth).
esp_err_t read_pico_hotspot_describe(cJSON *object);
// Count file transfers so the hotspot never closes during an upload.
void read_pico_hotspot_transfer(bool begin);
#ifdef __cplusplus
}
#endif
