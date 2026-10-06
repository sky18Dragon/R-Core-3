#include <string.h>

#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdcard.h"
#include "sdmmc_cmd.h"

static const char *TAG = "sdcard_sdio";

sdmmc_card_t *card_host = NULL;
static char mount_point_buf[32] = {0};
static esp_err_t unmount_failure;

esp_err_t sdcard_init(const sdcard_config_t *config)
{
    if (config == NULL || config->mount_point == NULL || config->mount_point[0] == '\0' ||
        strlen(config->mount_point) >= sizeof(mount_point_buf) ||
        (config->bus_width != SDCARD_SDMMC_BUS_WIDTH_1 &&
         config->bus_width != SDCARD_SDMMC_BUS_WIDTH_4) ||
        config->clk_pin == GPIO_NUM_NC || config->cmd_pin == GPIO_NUM_NC ||
        config->d0_pin == GPIO_NUM_NC ||
        (config->bus_width == SDCARD_SDMMC_BUS_WIDTH_4 &&
         (config->d1_pin == GPIO_NUM_NC || config->d2_pin == GPIO_NUM_NC ||
          config->d3_pin == GPIO_NUM_NC))) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }
    if (card_host != NULL || unmount_failure != ESP_OK) {
        ESP_LOGE(TAG, "SD card is already mounted");
        return ESP_ERR_INVALID_STATE;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024 * 3,
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = config->bus_width;
    slot_config.clk = config->clk_pin;
    slot_config.cmd = config->cmd_pin;
    slot_config.d0 = config->d0_pin;
    slot_config.d1 = config->bus_width == SDCARD_SDMMC_BUS_WIDTH_4 ? config->d1_pin : GPIO_NUM_NC;
    slot_config.d2 = config->bus_width == SDCARD_SDMMC_BUS_WIDTH_4 ? config->d2_pin : GPIO_NUM_NC;
    slot_config.d3 = config->bus_width == SDCARD_SDMMC_BUS_WIDTH_4 ? config->d3_pin : GPIO_NUM_NC;
    if (config->internal_pullups)
        slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG,
             "Mounting SD card via SDMMC width=%u (CLK=%d, CMD=%d, D0=%d, D1=%d, D2=%d, D3=%d)",
             (unsigned) config->bus_width, slot_config.clk, slot_config.cmd, slot_config.d0,
             slot_config.d1, slot_config.d2, slot_config.d3);

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(config->mount_point, &host, &slot_config, &mount_config,
                                            &card_host);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else if (ret == ESP_ERR_TIMEOUT || ret == ESP_ERR_NOT_FOUND || ret == 0x107) {
            ESP_LOGW(
                TAG,
                "SD card not detected or initialization failed (%s). Continuing in No-SDCard mode.",
                esp_err_to_name(ret));
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card (%s)", esp_err_to_name(ret));
        }
        card_host = NULL;
        return ret;
    }

    if (card_host != NULL) {
        strncpy(mount_point_buf, config->mount_point, sizeof(mount_point_buf) - 1);
        mount_point_buf[sizeof(mount_point_buf) - 1] = '\0';
        sdmmc_card_print_info(stdout, card_host);
        ESP_LOGI(TAG, "SD card mounted successfully at %s", mount_point_buf);
        return ESP_OK;
    }

    mount_point_buf[0] = '\0';
    return ESP_FAIL;
}

bool sdcard_is_mounted(void)
{
    return card_host != NULL;
}

esp_err_t sdcard_format(void)
{
    if (card_host == NULL) {
        ESP_LOGE(TAG, "Cannot format: no card mounted");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGW(TAG, "Formatting SD card at %s (all data will be erased)", mount_point_buf);
    esp_err_t ret = esp_vfs_fat_sdcard_format(mount_point_buf, card_host);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Format failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SD card formatted successfully");
    }
    return ret;
}

esp_err_t sdcard_deinit(void)
{
    if (unmount_failure != ESP_OK)
        return unmount_failure;
    if (card_host == NULL) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Unmounting SD card");
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(mount_point_buf, card_host);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Unmount returned %s", esp_err_to_name(ret));
        // IDF v6.1 frees context/card before VFS unregister returns. Keeping the
        // pointer would allow use-after-free; clearing alone would permit unsafe
        // remount over an unknown VFS state. Invalidate AND latch the failure.
        unmount_failure = ret;
    }
    card_host = NULL;
    mount_point_buf[0] = '\0';
    return ret;
}
