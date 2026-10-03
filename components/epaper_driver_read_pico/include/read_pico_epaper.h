#pragma once

#include "esp_err.h"
#include "read_pico_refresh.h"

#ifdef __cplusplus
extern "C" {
#endif

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
// Establish a reference only after the white clear is visually verified in this boot session.
esp_err_t read_pico_epaper_clear_unverified(void);
esp_err_t read_pico_epaper_confirm_white_reference(void);
bool read_pico_epaper_faulted(void);

#ifdef __cplusplus
}
#endif
