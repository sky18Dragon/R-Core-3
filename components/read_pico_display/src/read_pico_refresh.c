#include "read_pico_refresh.h"

read_pico_refresh_result_t read_pico_refresh_gc16(const read_pico_refresh_ops_t *ops,
                                                  read_pico_refresh_gate_t gate,
                                                  const uint8_t *source, size_t source_size,
                                                  uint8_t *scratch, size_t scratch_size)
{
    read_pico_refresh_result_t result = {.error = READ_PICO_REFRESH_INVALID_ARGUMENT};
    if (ops == NULL || ops->try_acquire == NULL || ops->release == NULL || ops->prepare == NULL ||
        ops->draw_gc16 == NULL || ops->stop == NULL)
        return result;
    if (!ops->try_acquire(ops->context)) {
        result.error = READ_PICO_REFRESH_BUSY;
        return result;
    }
    if (!gate.vcom_valid || gate.vcom_mv < 500 || gate.vcom_mv > 2500 || gate.vcom_mv % 10 != 0 ||
        (ops->read_temperature == NULL &&
         (!gate.temperature_valid || gate.temperature_c < 0 || gate.temperature_c > 50))) {
        result.error = READ_PICO_REFRESH_GATE_CLOSED;
        goto done;
    }
    // Require the exact canonical PhotoFrame frame, not an undocumented stride or trailing data.
    if (source_size != READ_PICO_FRAME_BYTES) {
        result.error = READ_PICO_REFRESH_BAD_FRAME;
        result.frame_error = source_size < READ_PICO_FRAME_BYTES ? READ_PICO_FRAME_BUFFER_TOO_SMALL
                                                                 : READ_PICO_FRAME_INVALID_ARGUMENT;
        goto done;
    }
    const read_pico_frame_layout_t layout = {READ_PICO_PANEL_WIDTH, READ_PICO_PANEL_HEIGHT,
                                             READ_PICO_PANEL_WIDTH / 2};
    result.frame_error = read_pico_frame_convert(source, source_size, layout, scratch, scratch_size,
                                                 layout, READ_PICO_ROTATE_0, false);
    if (result.frame_error != READ_PICO_FRAME_OK) {
        result.error = READ_PICO_REFRESH_BAD_FRAME;
        goto done;
    }
    result.operation_error = ops->prepare(ops->context, gate.vcom_mv);
    if (result.operation_error != 0) {
        result.error = READ_PICO_REFRESH_PREPARE_FAILED;
    } else {
        int temperature = gate.temperature_c;
        bool temperature_ok = true;
        if (ops->read_temperature != NULL) {
            // Sentinel rejects callbacks that return success without writing a sample.
            temperature = -1000;
            result.operation_error = ops->read_temperature(ops->context, &temperature);
            temperature_ok = result.operation_error == 0 && temperature >= 0 && temperature <= 50;
        }
        if (!temperature_ok) {
            result.error = READ_PICO_REFRESH_TEMPERATURE_FAILED;
        } else {
            result.operation_error =
                ops->draw_gc16(ops->context, scratch, READ_PICO_FRAME_BYTES, temperature);
            result.error =
                result.operation_error == 0 ? READ_PICO_REFRESH_OK : READ_PICO_REFRESH_DRAW_FAILED;
        }
    }
    // Always attempt cleanup after prepare, including a partially energized failure.
    result.cleanup_error = ops->stop(ops->context);
    if (result.cleanup_error != 0 && result.error == READ_PICO_REFRESH_OK)
        result.error = READ_PICO_REFRESH_STOP_FAILED;
    result.completed = result.error == READ_PICO_REFRESH_OK;
done:
    ops->release(ops->context);
    return result;
}
