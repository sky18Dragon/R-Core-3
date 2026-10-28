#include <stddef.h>

#include "esp_log.h"

// Integration remains link-only. The separate R1 candidate opts into the local
// product path; flashing that candidate requires its own hardware authorization.
extern void photoframe_app_main(void);

void app_main(void)
{
    void (*volatile product_entry)(void) = photoframe_app_main;
    if (product_entry == NULL) {
        ESP_LOGE("read_pico_integration", "PhotoFrame linkage missing");
        return;
    }
#if CONFIG_READ_PICO_LOCAL_FRAME
    ESP_LOGW("read_pico_integration", "R1_LOCAL_FRAME: starting local Web/SD frame; automatic boot clear");
    product_entry();
#else
    ESP_LOGW("read_pico_integration",
             "INTEGRATION_ONLY: PhotoFrame linked, runtime startup withheld. "
             "Requires SD/hardware acceptance, NVS migration, white reference and PMU policy. "
             "No board/storage/display/NVS/network startup performed.");
#endif
}
