#pragma once

#include "esp_err.h"
#include "read_pico_refresh.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bounded wait for the shared board lock when a refresh starts. Brief holders
// (touch polling, SD checks) must not make a refresh fail as busy.
#define READ_PICO_EPAPER_BUS_WAIT_MS 200

// Explicit capability, supplied only after power-write scope and board identity are approved.
// The P1 diagnostic always supplies false and does not invoke panel startup.
esp_err_t read_pico_epaper_init(bool runtime_power_config_authorized);
esp_err_t read_pico_epaper_deinit(void);
read_pico_refresh_result_t read_pico_epaper_display(const uint8_t *frame, size_t size);
// Only implemented in the isolated display-validation build. The input must be the
// portable full-screen gray16 control; compare real vendor drawing byte-for-byte before
// using its from-white entry. Requires untouched, manually confirmed white this boot.
read_pico_refresh_result_t read_pico_epaper_display_vendor_gray16(const uint8_t *frame,
                                                                  size_t size);
// First-boot clear has a void renderer API: it is not proof of a white physical screen.
read_pico_refresh_result_t read_pico_epaper_display_vendor_page(const uint8_t *frame, size_t size);
// Display-validation-only official power-condition control: two ordinary GC16 frames under one
// rail-on interval. Requires a fresh manually confirmed reference and never waits for user input
// while energized.
read_pico_refresh_result_t read_pico_epaper_display_continuous_pair(const uint8_t *white_frame,
                                                                    size_t white_size,
                                                                    const uint8_t *target_frame,
                                                                    size_t target_size);
// Establish a reference only after the white clear is visually verified in this boot session.
esp_err_t read_pico_epaper_clear_unverified(void);
esp_err_t read_pico_epaper_confirm_white_reference(void);
// R1-only product startup: initialize, clear once, and accept a SOFTWARE reference.
// Does not claim visual confirmation or a hard DMA timeout. Requires deployment
// authorization for automatic screen power/clear; diagnostics retain manual confirmation.
#if CONFIG_READ_PICO_LOCAL_FRAME
esp_err_t read_pico_epaper_prepare_local_frame(void);
#endif
bool read_pico_epaper_faulted(void);

#ifdef __cplusplus
}
#endif
