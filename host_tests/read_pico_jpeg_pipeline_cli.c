#include <stdio.h>

#include "config_manager.h"
#include "fake_display_manager.h"
#include "image_processor.h"
#include "processing_settings.h"

extern int test_board_display_width, test_board_display_height;
extern const char *test_board_display_type;
extern display_orientation_t test_display_orientation;
extern scale_mode_t test_scale_mode;
extern size_t test_heap_max_request, test_heap_request_limit;

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: read_pico_jpeg_pipeline_cli <read-only JPEG path>\n");
        return 2;
    }
    test_board_display_width = 1216;
    test_board_display_height = 684;
    test_board_display_type = "gc16";
    test_display_orientation = DISPLAY_ORIENTATION_PORTRAIT;
    test_scale_mode = SCALE_MODE_COVER;
    test_heap_request_limit = 5u * 1024u * 1024u;
    fake_display_reset();
    esp_err_t err = image_processor_init();
    if (err == ESP_OK)
        err = image_processor_process_jpeg_file_to_display(argv[1], DITHER_FLOYD_STEINBERG, NULL);
    bool palette = err == ESP_OK;
    const uint8_t *p = fake_display_frame();
    for (size_t i = 0; palette && i < 1216u * 684u * 3u; i += 3)
        palette = p[i] == p[i + 1] && p[i] == p[i + 2] && p[i] % 17 == 0;
    printf(
        "pipeline=%s dimensions=1216x684 cover=1 portrait=1 palette=%d max_heap_request=%zu "
        "fake_display=%d\n",
        esp_err_to_name(err), palette, test_heap_max_request, fake_display_was_shown());
    if (err != ESP_OK)
        fprintf(stderr, "%s\n", image_processor_get_last_error());
    fake_display_reset();
    return err == ESP_OK && palette ? 0 : 1;
}
