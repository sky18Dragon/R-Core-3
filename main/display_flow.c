#include "display_flow.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "storage.h"
#if CONFIG_BOARD_DRIVER_READ_PICO
#include "upload_asset.h"
#include "read_pico_sd.h"
#endif

static const char *TAG = "display_flow";

esp_err_t display_flow_read_file(const char *path, uint8_t **out_buf, size_t *out_size)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return ESP_FAIL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        fclose(fp);
        return ESP_FAIL;
    }

    uint8_t *buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    if (!buf) {
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    size_t read_bytes = fread(buf, 1, size, fp);
    fclose(fp);
    if (read_bytes != (size_t) size) {
        heap_caps_free(buf);
        return ESP_FAIL;
    }

    *out_buf = buf;
    *out_size = (size_t) size;
    return ESP_OK;
}

esp_err_t display_flow_stream_file(const char *path, image_format_t format,
                                   dither_algorithm_t algorithm, const display_publish_t *pub,
                                   bool release_source)
{
#if defined(CONFIG_BOARD_DRIVER_READ_PICO) && CONFIG_BOARD_DRIVER_READ_PICO
    if (!path || algorithm < DITHER_FLOYD_STEINBERG || algorithm > DITHER_SIERRA)
        return ESP_ERR_INVALID_ARG;
    // Only the checked persistent-file decoders may run on Read Pico. Reject
    // other formats before the legacy MemFS release_source/unlink branch.
    if (format != IMAGE_FORMAT_JPG && format != IMAGE_FORMAT_PNG)
        return ESP_ERR_NOT_SUPPORTED;
#endif
    if (format == IMAGE_FORMAT_PNG) {
        // File-backed fused path: no RAM copy of the source
        return image_processor_process_or_display_png(path, algorithm, pub, release_source);
    }
#if defined(CONFIG_BOARD_DRIVER_READ_PICO) && CONFIG_BOARD_DRIVER_READ_PICO
    if (format == IMAGE_FORMAT_JPG) {
        // Read Pico has persistent SD; do not materialize or retire the compressed source here.
        // Existing caller lifecycle handles temporary files after a successful display.
        return image_processor_process_jpeg_file_to_display(path, algorithm, pub);
    }
#endif

    uint8_t *buf = NULL;
    size_t size = 0;
    esp_err_t err = display_flow_read_file(path, &buf, &size);
    if (err != ESP_OK) {
        return err;
    }
    if (release_source) {
        // MemFS-backed source lives in PSRAM; drop the file now that the
        // compressed copy exists
        unlink(path);
    }
    err = image_processor_process_to_display(buf, size, format, algorithm, pub);
    heap_caps_free(buf);
    return err;
}

const char *display_flow_stage_file(const char *source_path, image_format_t format)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    // The old naked-path API cannot prove exclusive-create ownership. New
    // callers use present_asset; never retire a persistent source by name.
    (void)source_path;
    (void)format;
    return NULL;
#else
    const char *slot = (format == IMAGE_FORMAT_EPD_GZ) ? CURRENT_EPD_PATH : CURRENT_BMP_PATH;
    unlink(slot);
    if (rename(source_path, slot) != 0) {
        ESP_LOGE(TAG, "Failed to move %s into %s", source_path, slot);
        unlink(source_path);
        return NULL;
    }
    return slot;
#endif
}

void display_flow_retire_source(const char *source_path, image_format_t format,
                                bool fresh_thumbnail)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    (void)source_path;
    (void)format;
    (void)fresh_thumbnail;
    return;
#else
    if (storage_has_persistent_storage()) {
        if (format == IMAGE_FORMAT_JPG) {
            unlink(CURRENT_PNG_PATH);
            if (fresh_thumbnail) {
                // A thumbnail for this image claims the .current.jpg slot
                if (source_path) {
                    unlink(source_path);
                }
            } else if (source_path) {
                unlink(CURRENT_JPG_PATH);
                if (rename(source_path, CURRENT_JPG_PATH) != 0) {
                    // current_image will 404 rather than lie; the panel
                    // itself displayed fine
                    ESP_LOGW(TAG, "Failed to keep original JPEG as thumbnail");
                    unlink(source_path);
                }
            }
        } else {
            unlink(CURRENT_PNG_PATH);
            if (source_path && rename(source_path, CURRENT_PNG_PATH) != 0) {
                ESP_LOGW(TAG, "Failed to keep original PNG for current image");
                unlink(source_path);
            }
            if (!fresh_thumbnail) {
                unlink(CURRENT_JPG_PATH);  // stale thumbnail from an earlier display
            }
        }
    } else {
        // MemFS-backed /storage retains nothing full-size
        if (source_path) {
            unlink(source_path);
        }
        unlink(CURRENT_PNG_PATH);
        if (!fresh_thumbnail) {
            unlink(CURRENT_JPG_PATH);
        }
    }
    unlink(CURRENT_BMP_PATH);
    unlink(CURRENT_EPD_PATH);
#endif
}

void display_flow_drop_stale_current(const char *keep_path, bool keep_thumbnail)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    (void)keep_path;
    (void)keep_thumbnail;
    return;
#else
    bool keep = storage_has_persistent_storage() && keep_path != NULL;
    if (!keep || strcmp(keep_path, CURRENT_PNG_PATH) != 0) {
        unlink(CURRENT_PNG_PATH);
    }
    if (!keep || strcmp(keep_path, CURRENT_BMP_PATH) != 0) {
        unlink(CURRENT_BMP_PATH);
    }
    if (!keep || strcmp(keep_path, CURRENT_EPD_PATH) != 0) {
        unlink(CURRENT_EPD_PATH);
    }
    if (!keep_thumbnail) {
        unlink(CURRENT_JPG_PATH);
    }
#endif
}

#if CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t display_flow_publish_asset(upload_file_t *image, upload_file_t *preview,
                                     image_format_t format, upload_asset_t *asset)
{
    const char *ext = format == IMAGE_FORMAT_JPG ? ".jpg" :
                      format == IMAGE_FORMAT_PNG ? ".png" :
                      format == IMAGE_FORMAT_BMP ? ".bmp" :
                      format == IMAGE_FORMAT_EPD_GZ ? ".epdgz" : NULL;
    if (!ext || !image || !asset)
        return ESP_ERR_INVALID_ARG;
    if (!upload_asset_plan(image, preview, ext, asset))
        return ESP_ERR_INVALID_ARG;
    // Magic-byte checks identify format; the actual decoder still validates
    // the image. A close/media error here forbids publication and refresh.
    if (image_processor_detect_format(image->source) != format ||
        (preview && image_processor_detect_format(preview->source) != IMAGE_FORMAT_JPG))
        return ESP_ERR_NOT_SUPPORTED;
    if (!upload_asset_publish(image, preview, ext, asset))
        return ESP_FAIL;
    return ESP_OK;
}

esp_err_t display_flow_present_asset(upload_file_t *image, upload_file_t *preview,
                                     image_format_t format, dither_algorithm_t algorithm)
{
    if (algorithm < DITHER_FLOYD_STEINBERG || algorithm > DITHER_SIERRA)
        return ESP_ERR_INVALID_ARG;
    upload_asset_t asset;
    esp_err_t err = display_flow_publish_asset(image, preview, format, &asset);
    if (err != ESP_OK)
        return err;
    if (format == IMAGE_FORMAT_JPG || format == IMAGE_FORMAT_PNG) {
        display_publish_t pub = {.display_name = asset.image};
        return display_flow_stream_file(asset.image, format, algorithm, &pub, false);
    }
    return display_manager_show_image(asset.image);
}
#endif

FILE *display_flow_open_current(const char **out_content_type)
{
    if (!out_content_type) return NULL;
    char displayed[512] = {0};
#if CONFIG_BOARD_DRIVER_READ_PICO
    bool known = false;
    if (!display_manager_copy_current_image(displayed, sizeof(displayed), &known)) return NULL;
    if (!known) {
#endif
    FILE *link_fp = fopen(CURRENT_IMAGE_LINK, "r");
    if (!link_fp) {
        return NULL;
    }
    if (!fgets(displayed, sizeof(displayed), link_fp)) {
        if (fclose(link_fp) != 0) {
#if CONFIG_BOARD_DRIVER_READ_PICO
            read_pico_sd_note_file_close_failure();
#endif
        }
        return NULL;
    }
#if CONFIG_BOARD_DRIVER_READ_PICO
    bool good = !ferror(link_fp) && fgetc(link_fp) == EOF && !ferror(link_fp);
    if (fclose(link_fp) != 0) {
        read_pico_sd_note_file_close_failure();
        good = false;
    }
    if (!good) return NULL;
    }
    if (!displayed[0]) return NULL;
    read_pico_sd_status_t media;
    if (read_pico_sd_get_status(&media) != ESP_OK || !media.present || !media.mounted ||
        !media.media_valid || media.exit_failed) return NULL;
#else
    fclose(link_fp);
#endif

    size_t len = strlen(displayed);
    if (len > 0 && displayed[len - 1] == '\n') {
        displayed[len - 1] = '\0';
    }

    // The .jpg sibling of the displayed name is its thumbnail; serve that
    // when present. Selection happens by fopen so a concurrent display
    // update cannot invalidate the choice.
    char thumbnail[512];
    snprintf(thumbnail, sizeof(thumbnail), "%s", displayed);
    char *ext = strrchr(thumbnail, '.');
    if (ext && (strcasecmp(ext, ".bmp") == 0 || strcasecmp(ext, ".png") == 0 ||
                strcasecmp(ext, ".epdgz") == 0)) {
        strcpy(ext, ".jpg");
    }

    FILE *fp = NULL;
#if CONFIG_BOARD_DRIVER_READ_PICO
    char preview[512];
    if (upload_asset_preview_path(displayed, preview, sizeof(preview))) {
        fp = fopen(preview, "rb");
        if (fp) {
            *out_content_type = "image/jpeg";
            return fp;
        }
    }
#endif
    fp = fopen(thumbnail, "rb");
    if (fp) {
        *out_content_type = "image/jpeg";
        return fp;
    }

    const char *orig_ext = strrchr(displayed, '.');
    // Raw packed panel data is not renderable by a browser; report no
    // thumbnail rather than serving .epdgz bytes as an image
    if (orig_ext && strcasecmp(orig_ext, ".epdgz") == 0) {
        return NULL;
    }

    fp = fopen(displayed, "rb");
    if (!fp) {
        return NULL;
    }

    if (orig_ext && strcasecmp(orig_ext, ".png") == 0) {
        *out_content_type = "image/png";
    } else if (orig_ext && strcasecmp(orig_ext, ".bmp") == 0) {
        *out_content_type = "image/bmp";
    } else {
        *out_content_type = "image/jpeg";
    }
    return fp;
}
