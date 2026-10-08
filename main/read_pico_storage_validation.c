#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "board_hal.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "read_pico_board.h"
#include "read_pico_sd.h"
#include "sd_file_probe.h"
#include "read_pico_upload.h"
#include "upload_probe.h"
#include "jpeg_file_probe.h"

#if ESP_IDF_VERSION != ESP_IDF_VERSION_VAL(6, 1, 0)
#error "Read Pico SD validation requires the locked ESP-IDF v6.1.0 baseline"
#endif

static const char *TAG = "read_pico_storage";

static bool upload_target_absent(void *context)
{
    (void)context;
    struct stat info;
    errno = 0;
    return stat(UPLOAD_PROBE_TARGET, &info) != 0 && errno == ENOENT;
}
static int upload_open_read(void *context, const char *path)
{
    (void)context;
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        read_pico_sd_status_t status;
        esp_err_t err = read_pico_sd_get_status(&status);
        bool valid = err == ESP_OK && status.present && status.mounted && status.media_valid &&
                     !status.exit_failed && status.active_io == 1;
        ESP_LOGI(TAG, "Upload lease read-open=%s active_io=%u expected=1 valid=%d",
                 esp_err_to_name(err), status.active_io, valid);
        if (!valid) {
            if (close(fd) != 0) read_pico_sd_note_file_close_failure();
            errno = EIO;
            return -1;
        }
    }
    return fd;
}
static int upload_create(void *context, const char *path)
{
    upload_file_io_t base = read_pico_upload_io();
    int fd = base.create_new(context, path);
    if (fd >= 0) {
        read_pico_sd_status_t status;
        esp_err_t err = read_pico_sd_get_status(&status);
        bool valid = err == ESP_OK && status.present && status.mounted && status.media_valid &&
                     !status.exit_failed && status.active_io == 1;
        ESP_LOGI(TAG, "Upload lease create=%s active_io=%u expected=1 valid=%d",
                 esp_err_to_name(err), status.active_io, valid);
        if (!valid) {
            if (base.close(context, fd) != 0) read_pico_sd_note_file_close_failure();
            errno = EIO;
            return -1;
        }
    }
    return fd;
}
static int upload_close(void *context, int fd)
{
    upload_file_io_t base = read_pico_upload_io();
    int result = base.close(context, fd);
    if (result == 0) {
        read_pico_sd_status_t status;
        esp_err_t err = read_pico_sd_get_status(&status);
        bool valid = err == ESP_OK && status.present && status.mounted && status.media_valid &&
                     !status.exit_failed && status.active_io == 0;
        ESP_LOGI(TAG, "Upload lease closed=%s active_io=%u expected=0 valid=%d",
                 esp_err_to_name(err), status.active_io, valid);
        if (!valid) { errno = EIO; return -1; }
    }
    return result;
}
static int upload_read(void *context, int fd, uint8_t *data, size_t size)
{
    (void)context;
    return (int)read(fd, data, size);
}
static void run_upload_probe(upload_probe_session_t *session)
{
    upload_probe_io_t io = {read_pico_upload_io(), upload_target_absent, upload_open_read, upload_read};
    io.file.create_new = upload_create;
    io.file.close = upload_close;
    upload_probe_result_t r = upload_probe_run(session, &io);
    ESP_LOGI(TAG, "Upload probe=%s stage=%s created=%u written=%zu published=%u verified=%zu "
                 "conflict_rejected=%d close_failed=%d",
             r.ok ? "OK" : "FAILED", r.stage, r.created, r.written, r.published, r.verified,
             r.conflict_rejected, session->close_failed);
    ESP_LOGW(TAG, "Synthetic fixed files retained; no deletion/repair/overwrite/photo/display");
}

typedef struct {
    int first_errno;
    int close_errno;
    bool verify_lease; // Only file-check observes one owned descriptor, never writes.
} probe_context_t;

static int remember_error(probe_context_t *context, int result)
{
    if (result < 0 && context->first_errno == 0)
        context->first_errno = errno;
    return result;
}

static bool probe_media_ready(void *context)
{
    (void) context;
    read_pico_sd_status_t status;
    return read_pico_sd_get_status(&status) == ESP_OK && status.present && status.mounted &&
           status.media_valid && !status.exit_failed;
}

static int probe_open_new(void *context)
{
    // Locked IDF FAT VFS maps O_CREAT|O_EXCL to FA_CREATE_NEW (never truncates).
    return remember_error(context, open(SD_FILE_PROBE_PATH, O_WRONLY | O_CREAT | O_EXCL, 0600));
}

static int probe_open_read(void *context)
{
    probe_context_t *probe = context;
    int fd = remember_error(context, open(SD_FILE_PROBE_PATH, O_RDONLY));
    if (fd >= 0 && probe->verify_lease) {
        read_pico_sd_status_t status;
        esp_err_t err = read_pico_sd_get_status(&status);
        bool valid = err == ESP_OK && status.present && status.mounted && status.media_valid &&
                     !status.exit_failed && status.active_io == 1;
        ESP_LOGI(TAG, "File lease open observation=%s active_io=%u expected=1 valid=%d",
                 esp_err_to_name(err), status.active_io, valid);
        if (!valid) {
            // No file bytes consumed and no unmount experiment. Close exactly
            // once, then fail the existing one-shot read-only probe.
            if (close(fd) != 0) {
                probe->close_errno = errno;
                read_pico_sd_note_file_close_failure();
            }
            errno = EIO;
            return remember_error(context, -1);
        }
    }
    return fd;
}

static int probe_write(void *context, int fd, const uint8_t *data, size_t size)
{
    return remember_error(context, (int) write(fd, data, size));
}

static int probe_read(void *context, int fd, uint8_t *data, size_t size)
{
    return remember_error(context, (int) read(fd, data, size));
}

static int probe_sync(void *context, int fd)
{
    return remember_error(context, fsync(fd));
}

static int probe_close(void *context, int fd)
{
    int result = close(fd);
    probe_context_t *probe = context;
    if (probe->verify_lease && result == 0) {
        read_pico_sd_status_t status;
        esp_err_t err = read_pico_sd_get_status(&status);
        bool valid = err == ESP_OK && status.present && status.mounted && status.media_valid &&
                     !status.exit_failed && status.active_io == 0;
        ESP_LOGI(TAG, "File lease closed observation=%s active_io=%u expected=0 valid=%d",
                 esp_err_to_name(err), status.active_io, valid);
        if (!valid) {
            // A broken accounting/media postcondition is not normal teardown.
            errno = EIO;
            result = -1;
        }
    }
    if (result < 0)
        probe->close_errno = errno;
    return remember_error(context, result);
}

static void run_file_test(sd_file_probe_session_t *session)
{
    probe_context_t context = {0};
    const sd_file_probe_io_t io = {
        .context = &context, .media_ready = probe_media_ready,
        .open_new = probe_open_new, .open_read = probe_open_read,
        .write = probe_write, .read = probe_read, .sync = probe_sync, .close = probe_close,
    };
    sd_file_probe_result_t result = sd_file_probe_run(session, &io);
    ESP_LOGI(TAG, "File test=%s created=%d written=%zu verified=%zu close_failed=%d errno=%d close_errno=%d",
             sd_file_probe_error_name(result.error), result.created, result.written,
             result.verified, session->close_failed, context.first_errno, context.close_errno);
    if (result.created)
        ESP_LOGW(TAG, "Retained dedicated test file %s; no delete/overwrite", SD_FILE_PROBE_PATH);
    if (result.error == SD_FILE_PROBE_CREATE_FAILED && context.first_errno == EEXIST)
        ESP_LOGW(TAG, "Existing path refused; no contents read or changed");
}

static void report_status(void)
{
    read_pico_sd_status_t status;
    esp_err_t err = read_pico_sd_get_status(&status);
    ESP_LOGI(TAG, "SD observation=%s present=%d mounted=%d media_valid=%d exit_failed=%d active_io=%u",
             esp_err_to_name(err), status.present, status.mounted, status.media_valid,
             status.exit_failed, status.active_io);
}

static void run_file_check(sd_file_probe_session_t *session)
{
    probe_context_t context = {.verify_lease = true};
    const sd_file_probe_read_io_t io = {
        .context = &context, .media_ready = probe_media_ready,
        .open_read = probe_open_read, .read = probe_read, .close = probe_close,
    };
    sd_file_probe_result_t result = sd_file_probe_check(session, &io);
    ESP_LOGI(TAG, "File check=%s created=%d written=%zu verified=%zu close_failed=%d errno=%d close_errno=%d",
             sd_file_probe_error_name(result.error), result.created, result.written,
             result.verified, session->close_failed, context.first_errno, context.close_errno);
    ESP_LOGI(TAG, "Read-only check of %s; no create/write/repair/delete", SD_FILE_PROBE_PATH);
}

typedef struct { probe_context_t errors; unsigned calls; } photo_context_t;

static int photo_open(void *context)
{
    photo_context_t *photo = context;
    return remember_error(&photo->errors, open(JPEG_FILE_PROBE_PATH, O_RDONLY));
}
static int64_t photo_size(void *context, int fd)
{
    struct stat info;
    photo_context_t *photo = context;
    if (remember_error(&photo->errors, fstat(fd, &info)) < 0 || !S_ISREG(info.st_mode))
        return -1;
    return info.st_size;
}
static int photo_read(void *context, int fd, uint8_t *data, size_t size)
{
    photo_context_t *photo = context;
    // Give idle/watchdog a chance without changing display/PMU state.
    if ((++photo->calls & 127u) == 0)
        vTaskDelay(1);
    return remember_error(&photo->errors, (int) read(fd, data, size));
}
static int photo_close(void *context, int fd)
{
    photo_context_t *photo = context;
    return probe_close(&photo->errors, fd);
}
static void run_photo_check(jpeg_file_probe_session_t *session, sd_file_probe_session_t *files)
{
    if (files->close_failed)
        session->close_failed = true;
    photo_context_t context = {0};
    const jpeg_file_probe_io_t io = {
        .context = &context, .media_ready = probe_media_ready, .open_read = photo_open,
        .file_size = photo_size, .read = photo_read, .close = photo_close,
    };
    void *work = heap_caps_malloc(3100, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!work)
        ESP_LOGE(TAG, "JPEG work allocation failed; no source opened");
    jpeg_file_probe_result_t r = jpeg_file_probe_run(session, &io, work, 3100);
    heap_caps_free(work);
    if (session->close_failed)
        files->close_failed = true;
    ESP_LOGI(TAG, "Photo check=%s stream=%s decoder=%d file_size=%zu bytes_read=%zu codec_read=%zu "
             "source=%ux%u output=%ux%u pixels=%zu blocks=%zu close_failed=%d errno=%d close_errno=%d",
             jpeg_file_probe_error_name(r.error), jpeg_stream_probe_error_name(r.stream.error),
             r.stream.decoder_result, r.file_size, r.bytes_read, r.stream.bytes_read,
             r.stream.width, r.stream.height, r.stream.output_width, r.stream.output_height,
             r.stream.pixels, r.stream.blocks, session->close_failed,
             context.errors.first_errno, context.errors.close_errno);
    ESP_LOGW(TAG, "JPEG decode only at " JPEG_FILE_PROBE_PATH
             "; 1/4 scale, no EXIF/cover/dither/display/write/repair/delete");
}

void app_main(void)
{
    ESP_LOGW(TAG, "SD validation only: no boot mount/file IO/format/display/NVS/network");
    esp_err_t err = board_hal_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Board init failed: %s; entry ended", esp_err_to_name(err));
        return;
    }
    usb_serial_jtag_driver_config_t config = {.tx_buffer_size = 2048, .rx_buffer_size = 256};
    // Do not borrow/delete a console driver owned by another startup path.
    if (usb_serial_jtag_is_driver_installed()) {
        ESP_LOGE(TAG, "Console already owned; no SD command accepted");
        err = read_pico_board_deinit();
        ESP_LOGI(TAG, "HAL cleanup=%s", esp_err_to_name(err));
        return;
    }
    err = usb_serial_jtag_driver_install(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Console install failed: %s", esp_err_to_name(err));
        err = read_pico_board_deinit();
        ESP_LOGI(TAG, "HAL cleanup=%s", esp_err_to_name(err));
        return;
    }
    usb_serial_jtag_vfs_use_driver();
    report_status();
    ESP_LOGW(TAG, "READY: status / mount / file-test / file-check / upload-test / photo-check / stop; each file command once at %s",
             SD_FILE_PROBE_PATH);
    read_pico_sd_parser_t parser = {0};
    sd_file_probe_session_t probe_session = {0};
    jpeg_file_probe_session_t photo_session = {0};
    upload_probe_session_t upload_session = {0};
    bool stopped = false;
    while (!stopped) {
        unsigned char byte;
        if (usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100)) != 1)
            continue;
        switch (read_pico_sd_parser_feed(&parser, byte)) {
        case READ_PICO_SD_COMMAND_MOUNT:
            err = read_pico_sd_mount();
            ESP_LOGI(TAG, "Mount=%s (one attempt)", esp_err_to_name(err));
            report_status();
            break;
        case READ_PICO_SD_COMMAND_STATUS:
            report_status();
            break;
        case READ_PICO_SD_COMMAND_FILE_TEST:
            run_file_test(&probe_session);
            report_status();
            break;
        case READ_PICO_SD_COMMAND_FILE_CHECK:
            run_file_check(&probe_session);
            report_status();
            break;
        case READ_PICO_SD_COMMAND_UPLOAD_TEST:
            run_upload_probe(&upload_session);
            probe_session.close_failed |= upload_session.close_failed;
            report_status();
            break;
        case READ_PICO_SD_COMMAND_PHOTO_CHECK:
            run_photo_check(&photo_session, &probe_session);
            report_status();
            break;
        case READ_PICO_SD_COMMAND_STOP:
            // The synchronous probe closes its handles before returning. An ambiguous
            // close error must not be hidden by teardown; retain the lease until reset.
            err = probe_session.close_failed ? ESP_ERR_INVALID_STATE : read_pico_sd_unmount();
            if (probe_session.close_failed)
                ESP_LOGE(TAG, "File close failure latched; unmount/HAL exit refused, restart required");
            ESP_LOGI(TAG, "Unmount=%s", esp_err_to_name(err));
            if (err == ESP_OK) {
                err = read_pico_board_deinit();
                ESP_LOGI(TAG, "HAL cleanup=%s", esp_err_to_name(err));
            }
            ESP_LOGW(TAG, "%s; command processing ended", err == ESP_OK ? "STOPPED" : "FAILED");
            stopped = true;  // Terminal even on cleanup failure; restart required.
            break;
        case READ_PICO_SD_COMMAND_INVALID:
            ESP_LOGW(TAG, "Rejected command line; no operation");
            break;
        case READ_PICO_SD_COMMAND_NONE:
            break;
        }
    }
    (void) usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(200));
    usb_serial_jtag_vfs_use_nonblocking();
    err = usb_serial_jtag_driver_uninstall();
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Console cleanup failed: %s", esp_err_to_name(err));
}
