#include "storage.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "config_manager.h"
#include "esp_err.h"
#include "esp_log.h"

#if defined(CONFIG_HAS_SDCARD) && !defined(CONFIG_BOARD_DRIVER_READ_PICO)
#include "sdcard.h"
#endif
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
#include "read_pico_sd.h"
#endif

#ifdef CONFIG_USE_INTERNAL_FLASH_STORAGE
#include "esp_littlefs.h"
#include "esp_partition.h"
#endif

#include <sys/stat.h>

#include "album_manager.h"
#include "memfs.h"
#include "utils.h"

static const char *TAG = "storage";
static storage_type_t current_storage_type = STORAGE_TYPE_NONE;

#ifdef CONFIG_USE_INTERNAL_FLASH_STORAGE
static esp_err_t register_littlefs(bool grow_on_mount)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = FS_MOUNT_POINT,
        .partition_label = LITTLEFS_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
        .grow_on_mount = grow_on_mount,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
    }
    return ret;
}

static esp_err_t mount_littlefs(void)
{
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_err_t ret = register_littlefs(false);
    if (ret != ESP_OK) {
        return ret;
    }

    // The storage partition reaches the end of flash since v2.19.0, and a
    // full reflash writes the new partition table while leaving the
    // filesystem bytes in place. The mount above autodetects the size
    // recorded in the superblock, so it succeeds either way; reconcile the
    // filesystem with the partition it now lives in:
    //  - filesystem smaller (reflash from an older layout): remount with
    //    grow_on_mount, which expands it in place and keeps the photos
    //  - filesystem larger (reflash to a layout with a smaller storage
    //    partition): littlefs cannot shrink and lfs_fs_grow() asserts on a
    //    smaller size, so reformat instead
    const esp_partition_t *partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, LITTLEFS_PARTITION_LABEL);
    size_t fs_size = 0, fs_used = 0;
    if (partition != NULL &&
        esp_littlefs_info(LITTLEFS_PARTITION_LABEL, &fs_size, &fs_used) == ESP_OK &&
        fs_size != partition->size) {
        esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
        if (fs_size < partition->size) {
            ESP_LOGI(TAG, "Growing filesystem to fill partition: %u -> %u bytes",
                     (unsigned) fs_size, (unsigned) partition->size);
            ret = register_littlefs(true);
            if (ret != ESP_OK) {
                // A failed grow must not take out otherwise-usable storage
                ESP_LOGW(TAG, "Grow failed; mounting at the current size");
                ret = register_littlefs(false);
            }
        } else {
            ESP_LOGW(TAG, "Filesystem (%u bytes) exceeds partition (%u bytes), reformatting",
                     (unsigned) fs_size, (unsigned) partition->size);
            esp_littlefs_format(LITTLEFS_PARTITION_LABEL);
            ret = register_littlefs(false);
        }
        if (ret != ESP_OK) {
            return ret;
        }
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(LITTLEFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LittleFS Partition size: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}
#endif

esp_err_t storage_init(void)
{
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    if (current_storage_type != STORAGE_TYPE_NONE) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t ret = read_pico_sd_mount();
    if (ret == ESP_OK) {
        current_storage_type = STORAGE_TYPE_SDCARD;
    }
    // No automatic format, second mount attempt or fallback on this board.
    return ret;
#else
    esp_err_t ret = ESP_OK;

#ifdef CONFIG_HAS_SDCARD
    // For devices with SD card configured, SD card handles its own init in board_hal_init.
    // We just check if it was successfully mounted.
    if (sdcard_is_mounted()) {
        ESP_LOGI(TAG, "SD Card storage is active");
        current_storage_type = STORAGE_TYPE_SDCARD;
        return ESP_OK;
    }

    ESP_LOGW(TAG, "SD Card not mounted. Attempting fallback...");
#endif

#ifdef CONFIG_USE_INTERNAL_FLASH_STORAGE
    // Fallback or explicit flash storage
    if (mount_littlefs() == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS storage is active");
        current_storage_type = STORAGE_TYPE_LITTLEFS;
        return ESP_OK;
    }
#endif

    // Final fallback to MemFS
    ESP_LOGW(TAG, "No persistent storage available, mounting MemFS at %s", FS_MOUNT_POINT);
    ret = memfs_mount(FS_MOUNT_POINT, 10);
    if (ret == ESP_OK) {
        current_storage_type = STORAGE_TYPE_MEMFS;
    } else {
        ESP_LOGE(TAG, "Failed to mount MemFS fallback!");
    }

    return ret;
#endif
}

storage_type_t storage_get_type(void)
{
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    read_pico_sd_status_t status;
    if (current_storage_type == STORAGE_TYPE_SDCARD &&
        (read_pico_sd_get_status(&status) != ESP_OK || !status.present || !status.mounted ||
         !status.media_valid || status.exit_failed)) {
        return STORAGE_TYPE_NONE;
    }
#endif
    return current_storage_type;
}

bool storage_has_persistent_storage(void)
{
    storage_type_t type = storage_get_type();
    return type == STORAGE_TYPE_SDCARD || type == STORAGE_TYPE_LITTLEFS;
}

void storage_unmount(void)
{
    esp_err_t ret = storage_unmount_checked();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Storage unmount failed (%s); ownership retained", esp_err_to_name(ret));
    }
}

esp_err_t storage_unmount_checked(void)
{
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    // Use ownership, not media availability: removed/invalid media still needs teardown.
    if (current_storage_type == STORAGE_TYPE_SDCARD) {
        esp_err_t ret = read_pico_sd_unmount();
        if (ret != ESP_OK) {
            return ret;
        }
        current_storage_type = STORAGE_TYPE_NONE;
    }
#endif
#ifdef CONFIG_USE_INTERNAL_FLASH_STORAGE
    if (current_storage_type == STORAGE_TYPE_LITTLEFS) {
        ESP_LOGI(TAG, "Unmounting LittleFS before deep sleep");
        esp_err_t ret = esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
        if (ret != ESP_OK) {
            return ret;
        }
        current_storage_type = STORAGE_TYPE_NONE;
    }
#endif
    return ESP_OK;
}

esp_err_t storage_format(void)
{
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    // Product format requires a separately reviewed destructive-operation policy.
    return ESP_ERR_NOT_SUPPORTED;
#else
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;

    switch (current_storage_type) {
#ifdef CONFIG_USE_INTERNAL_FLASH_STORAGE
    case STORAGE_TYPE_LITTLEFS:
        ESP_LOGW(TAG, "Formatting LittleFS partition...");
        // Unmount, format, and remount
        esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
        ret = esp_littlefs_format(LITTLEFS_PARTITION_LABEL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to format LittleFS: %s", esp_err_to_name(ret));
            // Try to remount anyway
            mount_littlefs();
            return ret;
        }
        ESP_LOGI(TAG, "LittleFS formatted successfully, remounting...");
        ret = mount_littlefs();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to remount LittleFS after format");
            return ret;
        }
        break;
#endif
#ifdef CONFIG_HAS_SDCARD
    case STORAGE_TYPE_SDCARD:
        // SD card stays mounted across format; only the filesystem is
        // reinitialised as fresh FAT32.
        ret = sdcard_format();
        if (ret != ESP_OK) {
            return ret;
        }
        break;
#endif
    default:
        ESP_LOGE(TAG, "Format not supported for current storage type (%d)", current_storage_type);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Recreate the images directory and default album regardless of backend
    mkdir(IMAGE_DIRECTORY, 0775);
    album_manager_ensure_default_album();

    ESP_LOGI(TAG, "Storage format complete");
    return ESP_OK;
#endif
}

#ifdef CONFIG_BOARD_DRIVER_READ_PICO
esp_err_t storage_read_wifi_config(wifi_text_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    if (!storage_has_persistent_storage()) return ESP_ERR_INVALID_STATE;
    const char *paths[] = {FS_MOUNT_POINT "/config/wifi.txt", FS_MOUNT_POINT "/wifi.txt"};
    FILE *fp = NULL;
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        errno = 0;
        fp = fopen(paths[i], "rb");
        if (fp) break;
        if (errno != ENOENT) return ESP_FAIL; // Do not hide IO/access errors with root fallback.
    }
    if (!fp) return storage_has_persistent_storage() ? ESP_ERR_NOT_FOUND : ESP_ERR_INVALID_STATE;
    struct stat st;
    uint8_t bytes[WIFI_TEXT_MAX_BYTES];
    wifi_text_t parsed;
    esp_err_t err = ESP_OK;
    if (fstat(fileno(fp), &st) != 0 || !S_ISREG(st.st_mode)) err = ESP_FAIL;
    else if (st.st_size <= 0 || st.st_size > sizeof(bytes)) err = ESP_ERR_INVALID_SIZE;
    else if (!storage_has_persistent_storage() ||
             fread(bytes, 1, (size_t)st.st_size, fp) != (size_t)st.st_size || ferror(fp)) err = ESP_FAIL;
    else if (fgetc(fp) != EOF || !feof(fp) || ferror(fp)) err = ESP_FAIL;
    else if (!wifi_text_parse(bytes, (size_t)st.st_size, &parsed)) err = ESP_ERR_INVALID_ARG;
    if (fclose(fp) != 0) {
        read_pico_sd_note_file_close_failure();
        err = ESP_FAIL;
    }
    if (!storage_has_persistent_storage()) err = ESP_ERR_INVALID_STATE;
    if (err == ESP_OK) *out = parsed;
    return err; // No credential/name save, unlink, retry or source modification.
}
#else
esp_err_t storage_read_wifi_config(wifi_text_t *out)
{
    (void)out;
    return ESP_ERR_NOT_SUPPORTED;
}
#endif

esp_err_t storage_read_wifi_credentials(char *ssid, char *password)
{
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    if (!ssid || !password) return ESP_ERR_INVALID_ARG;
    wifi_text_t parsed;
    esp_err_t err = storage_read_wifi_config(&parsed);
    if (err != ESP_OK) return err;
    memcpy(ssid, parsed.ssid, sizeof(parsed.ssid));
    memcpy(password, parsed.password, sizeof(parsed.password));
    return ESP_OK;
#else
#ifdef CONFIG_HAS_SDCARD
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    if (!storage_has_persistent_storage()) {
#else
    if (!sdcard_is_mounted()) {
#endif
        return ESP_ERR_NOT_FOUND;
    }

    // Try multiple possible locations, starting with the safest (config folder)
    const char *paths[] = {FS_MOUNT_POINT "/config/wifi.txt", FS_MOUNT_POINT "/wifi.txt", NULL};

    FILE *f = NULL;
    for (int i = 0; paths[i] != NULL; i++) {
        f = fopen(paths[i], "r");
        if (f) {
            ESP_LOGI(TAG, "Found WiFi credentials file at: %s", paths[i]);
            break;
        }
    }

    if (!f) {
        return ESP_ERR_NOT_FOUND;
    }

    char line[128];
    int line_num = 0;

    // Read file line by line
    while (fgets(line, sizeof(line), f) != NULL && line_num < 3) {
        // Remove trailing newline and carriage return
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }

        switch (line_num) {
        case 0:  // SSID
            strncpy(ssid, line, WIFI_SSID_MAX_LEN - 1);
            ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
            break;
        case 1:  // Password
            strncpy(password, line, WIFI_PASS_MAX_LEN - 1);
            password[WIFI_PASS_MAX_LEN - 1] = '\0';
            break;
        case 2:  // Device Name (Optional)
            config_manager_set_device_name(line);
            ESP_LOGI(TAG, "Loaded Device Name from SD: %s", line);
            break;
        }
        line_num++;
    }
    fclose(f);

    if (line_num >= 2) {
        ESP_LOGI(TAG, "Successfully read WiFi credentials from SD card");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Invalid wifi.txt format on SD card");
        return ESP_ERR_INVALID_ARG;
    }
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
#endif
}
