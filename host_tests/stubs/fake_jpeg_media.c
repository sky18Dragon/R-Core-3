#include <stdint.h>
#undef fclose
#include <stdio.h>

#include "read_pico_sd.h"

bool test_jpeg_media_present = true;
size_t test_jpeg_media_calls;
size_t test_jpeg_media_fail_at = SIZE_MAX;
size_t test_heap_max_request;
size_t test_heap_request_limit = SIZE_MAX;
bool test_jpeg_close_failed;
size_t test_image_close_calls;
size_t test_image_close_fail_at = SIZE_MAX;
size_t test_image_close_media_loss_at = SIZE_MAX;

int read_pico_test_fclose(FILE *file)
{
    size_t call = test_image_close_calls++;
    int result = fclose(file);
    if (call == test_image_close_media_loss_at)
        test_jpeg_media_present = false;
    // Actually release the host handle before emulating an uncertain FAT close.
    return call == test_image_close_fail_at ? -1 : result;
}

void read_pico_sd_note_file_close_failure(void)
{
    test_jpeg_close_failed = true;
}

esp_err_t read_pico_sd_get_status(read_pico_sd_status_t *status)
{
    bool ready = !test_jpeg_close_failed && test_jpeg_media_present &&
                 test_jpeg_media_calls++ < test_jpeg_media_fail_at;
    *status = (read_pico_sd_status_t) {.present = ready,
                                       .mounted = true,
                                       .media_valid = ready,
                                       .exit_failed = test_jpeg_close_failed};
    return ESP_OK;
}
