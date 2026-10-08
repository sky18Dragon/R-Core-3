#include "read_pico_sd.h"
#include <stdatomic.h>
#include <limits.h>

#include "board_read_pico.h"
#include "read_pico_board.h"
#include "sdcard.h"

static bool owned;
static bool media_valid;
static esp_err_t exit_failure;
static atomic_bool file_close_failed;
static atomic_uint active_io;

void read_pico_sd_note_file_close_failure(void)
{
    atomic_store(&file_close_failed, true);
}

esp_err_t read_pico_sd_io_begin(void)
{
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    if (!owned || !media_valid || exit_failure != ESP_OK ||
        atomic_load(&file_close_failed) || !sdcard_is_mounted()) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        bool present = false;
        err = read_pico_board_sd_present(&present);
        if (err == ESP_OK && !present)
            err = ESP_ERR_NOT_FOUND;
        if (err != ESP_OK)
            media_valid = false;
        else if (atomic_load(&active_io) == UINT_MAX)
            err = ESP_ERR_INVALID_STATE;
        else
            atomic_fetch_add(&active_io, 1);
    }
    read_pico_board_unlock();
    return err;
}

void read_pico_sd_io_end(void)
{
    unsigned count = atomic_load(&active_io);
    while (count) {
        if (atomic_compare_exchange_weak(&active_io, &count, count - 1))
            return;
    }
    // Never wrap the count or silently permit teardown after ownership misuse.
    read_pico_sd_note_file_close_failure();
}

esp_err_t read_pico_sd_mount(void)
{
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    if (atomic_load(&file_close_failed) || atomic_load(&active_io) || owned ||
        sdcard_is_mounted() || exit_failure != ESP_OK) {
        read_pico_board_unlock();
        return ESP_ERR_INVALID_STATE;
    }
    bool present = false;
    err = read_pico_board_sd_present(&present);
    if (err == ESP_OK && !present)
        err = ESP_ERR_NOT_FOUND;
    if (err == ESP_OK)
        err = read_pico_board_storage_claim();
    if (err == ESP_OK) {
        owned = true;
        const sdcard_config_t config = {
            .mount_point = READ_PICO_SD_MOUNT_POINT,
            .bus_width = BOARD_HAL_SD_BUS_WIDTH,
            .internal_pullups = true,
            .clk_pin = BOARD_HAL_SD_CLK_PIN,
            .cmd_pin = BOARD_HAL_SD_CMD_PIN,
            .d0_pin = BOARD_HAL_SD_D0_PIN,
            .d1_pin = BOARD_HAL_SD_D1_PIN,
            .d2_pin = BOARD_HAL_SD_D2_PIN,
            .d3_pin = BOARD_HAL_SD_D3_PIN,
        };
        err = sdcard_init(&config);  // Exactly one attempt; never auto-format/retry.
        if (err == ESP_OK) {
            err = read_pico_board_sd_present(&present);
            if (err == ESP_OK && !present)
                err = ESP_ERR_NOT_FOUND;
        }
        if (err == ESP_OK) {
            media_valid = true;
        } else {
            // Handles have not been published to any consumer yet.
            esp_err_t cleanup = sdcard_deinit();
            if (cleanup == ESP_OK)
                cleanup = read_pico_board_storage_release();
            if (cleanup == ESP_OK)
                owned = false;
            else
                exit_failure = cleanup;
        }
    }
    read_pico_board_unlock();
    return err;
}

esp_err_t read_pico_sd_unmount(void)
{
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    if (atomic_load(&file_close_failed) || atomic_load(&active_io))
        err = ESP_ERR_INVALID_STATE;
    else if (exit_failure != ESP_OK)
        err = exit_failure;
    else if (owned) {
        media_valid = false;
        err = sdcard_deinit();
        if (err == ESP_OK)
            err = read_pico_board_storage_release();
        if (err == ESP_OK)
            owned = false;
        else
            exit_failure = err;  // Keep HAL lease; no retry of possibly freed handles.
    } else if (sdcard_is_mounted()) {
        err = ESP_ERR_INVALID_STATE;  // Never tear down somebody else's mount.
    }
    read_pico_board_unlock();
    return err;
}

esp_err_t read_pico_sd_get_status(read_pico_sd_status_t *out)
{
    if (out == NULL)
        return ESP_ERR_INVALID_ARG;
    *out = (read_pico_sd_status_t) {0};
    esp_err_t err = read_pico_board_lock(100);
    if (err != ESP_OK)
        return err;
    err = read_pico_board_sd_present(&out->present);
    if (atomic_load(&file_close_failed))
        media_valid = false;
    if (owned && (err != ESP_OK || !out->present))
        media_valid = false;
    out->mounted = owned && sdcard_is_mounted();
    out->media_valid = out->mounted && media_valid && exit_failure == ESP_OK;
    out->exit_failed = exit_failure != ESP_OK || atomic_load(&file_close_failed);
    out->active_io = atomic_load(&active_io);
    read_pico_board_unlock();
    return err;
}
