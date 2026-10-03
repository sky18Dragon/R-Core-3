#pragma once
#include <stdbool.h>
#include <stdint.h>
// Host boundary models, NOT replacement vendor implementation/signature validation.
typedef struct {
    bool ep_mode, ep_output_enable;
} epd_ctrl_state_t;
typedef struct {
    void (*init)(uint32_t);
    void (*deinit)(void);
    void (*set_ctrl)(epd_ctrl_state_t *, const epd_ctrl_state_t *);
    void (*poweron)(epd_ctrl_state_t *);
    void (*poweroff)(epd_ctrl_state_t *);
    float (*get_temperature)(void);
} EpdBoardDefinition;
typedef struct {
    int unused;
} EpdWaveform;
typedef struct {
    int unused;
} EpdDisplay_t;
typedef struct {
    int x, y, width, height;
} EpdRect;
typedef struct {
    uint8_t *front_fb, *back_fb, *difference_fb, *dirty_columns;
    bool *dirty_lines;
    const EpdWaveform *waveform;
} EpdiyHighlevelState;
typedef struct {
    int clock, ckv, leh, start_pulse, stv;
    int data[16];
} lcd_bus_config_t;
enum { EPD_LUT_1K = 1, EPD_ROT_LANDSCAPE = 0, EPD_ROT_INVERTED_PORTRAIT = 3, MODE_GC16 = 2 };
enum EpdDrawError { EPD_DRAW_SUCCESS = 0, MOCK_DRAW_FAILURE = 7 };
#ifdef __cplusplus
extern "C" {
#endif
void epd_init(const EpdBoardDefinition *, const EpdDisplay_t *, int);
void epd_deinit(void);
void epd_lcd_deinit(void);
void epd_set_rotation(int);
int epd_get_rotation(void);
int epd_width(void);
int epd_height(void);
void epd_fill_rect(EpdRect, uint8_t, uint8_t *);
void epd_set_leading_skip(bool);
void epd_poweron(void);
void epd_clear(void);
enum EpdDrawError epd_hl_update_screen_full(EpdiyHighlevelState *, int, int);
enum EpdDrawError epd_hl_update_screen_from_white(EpdiyHighlevelState *, int, int);
#ifdef __cplusplus
}
#endif
