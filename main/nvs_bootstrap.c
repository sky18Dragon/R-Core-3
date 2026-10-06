#include "nvs_bootstrap.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
#include "nvs_flash.h"

esp_err_t photoframe_nvs_init(bool allow_erase)
{
    esp_err_t err = nvs_flash_init();
#ifdef CONFIG_BOARD_DRIVER_READ_PICO
    // Compile out erase recovery even if a future caller mistakenly opts in.
    (void) allow_erase;
    return err;
#else
    if (!allow_erase || (err != ESP_ERR_NVS_NO_FREE_PAGES && err != ESP_ERR_NVS_NEW_VERSION_FOUND))
        return err;
    err = nvs_flash_erase();
    return err == ESP_OK ? nvs_flash_init() : err;
#endif
}
