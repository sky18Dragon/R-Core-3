#include <stddef.h>

#include "esp_log.h"

// Keep the real PhotoFrame startup reachable to the linker for integration checks,
// but do not activate it before migration and real hardware gates are accepted.
extern void photoframe_app_main(void);

void app_main(void)
{
    void (*volatile product_entry)(void) = photoframe_app_main;
    if (product_entry == NULL) {
        ESP_LOGE("read_pico_integration", "PhotoFrame linkage missing");
        return;
    }
    ESP_LOGW("read_pico_integration",
             "INTEGRATION_ONLY: PhotoFrame linked, runtime startup withheld. "
             "Requires SD/hardware acceptance, NVS migration, white reference and PMU policy. "
             "No board/storage/display/NVS/network startup performed.");
}
