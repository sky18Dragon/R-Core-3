#include <stdio.h>

#include "image_processor.h"
#include "read_pico_photo_bridge.h"
#include "read_pico_photo_frame.h"

extern int test_board_display_width, test_board_display_height;
extern const char *test_board_display_type;
extern bool test_jpeg_media_present;
extern size_t test_jpeg_media_calls, test_jpeg_media_fail_at;
extern size_t test_heap_max_request, test_heap_request_limit;

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("Usage: read_pico_photo_diagnostic_cli <explicitly authorized JPEG>\n", stderr);
        return 2;
    }
    test_board_display_width = 1216;
    test_board_display_height = 684;
    test_board_display_type = "gc16";
    test_jpeg_media_present = true;
    test_jpeg_media_calls = 0;
    test_jpeg_media_fail_at = SIZE_MAX;
    test_heap_max_request = 0;
    test_heap_request_limit = 5u * 1024u * 1024u;
    esp_err_t err = image_processor_init_defaults();
    if (err == ESP_OK)
        err = image_processor_process_jpeg_file_to_display(argv[1], DITHER_FLOYD_STEINBERG, NULL);
    const uint8_t *pixels = read_pico_photo_bridge_frame();
    size_t histogram[16] = {0}, total = 0;
    if (pixels)
        for (size_t i = 0; i < READ_PICO_FRAME_BYTES; ++i) {
            ++histogram[pixels[i] >> 4];
            ++histogram[pixels[i] & 15];
            total += 2;
        }
    printf(
        "processing=%s packed_bytes=%u pixels=%zu max_heap_request=%zu fake_display=1 "
        "hardware_io=0\n",
        esp_err_to_name(err), pixels ? READ_PICO_FRAME_BYTES : 0, total, test_heap_max_request);
    if (pixels) {
        fputs("gray_histogram=", stdout);
        for (unsigned i = 0; i < 16; ++i)
            printf("%s%zu", i ? "," : "", histogram[i]);
        putchar('\n');
    }
    read_pico_photo_bridge_release();
    return err == ESP_OK && total == READ_PICO_PANEL_WIDTH * READ_PICO_PANEL_HEIGHT ? 0 : 1;
}
