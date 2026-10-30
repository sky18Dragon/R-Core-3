#include "read_pico_refresh.h"

static read_pico_refresh_result_t refresh_gc16_sequence(const read_pico_refresh_ops_t *ops,
                                                        read_pico_refresh_gate_t gate,
                                                        const uint8_t *const *sources,
                                                        const size_t *source_sizes,
                                                        size_t frame_count, uint8_t *scratch,
                                                        size_t scratch_size)
{
    read_pico_refresh_result_t result = {.error = READ_PICO_REFRESH_INVALID_ARGUMENT};
    if (ops == NULL || ops->try_acquire == NULL || ops->release == NULL || ops->prepare == NULL ||
        ops->draw_gc16 == NULL || ops->stop == NULL || sources == NULL || source_sizes == NULL ||
        frame_count == 0 || frame_count > 2)
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
    const read_pico_frame_layout_t layout = {READ_PICO_PANEL_WIDTH, READ_PICO_PANEL_HEIGHT,
                                             READ_PICO_PANEL_WIDTH / 2};
    // Validate every input before power-on. Reusing scratch is intentional; the first frame is
    // converted again below after pair preflight leaves the second frame in scratch.
    for (size_t i = 0; i < frame_count; ++i) {
        if (source_sizes[i] != READ_PICO_FRAME_BYTES) {
            result.error = READ_PICO_REFRESH_BAD_FRAME;
            result.frame_error = source_sizes[i] < READ_PICO_FRAME_BYTES
                                     ? READ_PICO_FRAME_BUFFER_TOO_SMALL
                                     : READ_PICO_FRAME_INVALID_ARGUMENT;
            goto done;
        }
        result.frame_error =
            read_pico_frame_convert(sources[i], source_sizes[i], layout, scratch, scratch_size,
                                    layout, READ_PICO_ROTATE_0, false);
        if (result.frame_error != READ_PICO_FRAME_OK) {
            result.error = READ_PICO_REFRESH_BAD_FRAME;
            goto done;
        }
    }
    if (frame_count > 1) {
        result.frame_error =
            read_pico_frame_convert(sources[0], source_sizes[0], layout, scratch, scratch_size,
                                    layout, READ_PICO_ROTATE_0, false);
        if (result.frame_error != READ_PICO_FRAME_OK) {
            result.error = READ_PICO_REFRESH_BAD_FRAME;
            goto done;
        }
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
            result.error = READ_PICO_REFRESH_OK;
            for (size_t i = 0; i < frame_count; ++i) {
                if (i > 0) {
                    result.frame_error =
                        read_pico_frame_convert(sources[i], source_sizes[i], layout, scratch,
                                                scratch_size, layout, READ_PICO_ROTATE_0, false);
                    if (result.frame_error != READ_PICO_FRAME_OK) {
                        result.error = READ_PICO_REFRESH_BAD_FRAME;
                        break;
                    }
                }
                result.operation_error =
                    ops->draw_gc16(ops->context, scratch, READ_PICO_FRAME_BYTES, temperature);
                if (result.operation_error != 0) {
                    result.error = READ_PICO_REFRESH_DRAW_FAILED;
                    break;
                }
                ++result.frames_completed;
            }
        }
    }
    // Always attempt cleanup after prepare, including a partially energized failure.
    result.cleanup_error = ops->stop(ops->context);
    if (result.cleanup_error != 0 && result.error == READ_PICO_REFRESH_OK)
        result.error = READ_PICO_REFRESH_STOP_FAILED;
    result.completed =
        result.error == READ_PICO_REFRESH_OK && result.frames_completed == frame_count;
done:
    ops->release(ops->context);
    return result;
}

read_pico_refresh_result_t read_pico_refresh_gc16(const read_pico_refresh_ops_t *ops,
                                                  read_pico_refresh_gate_t gate,
                                                  const uint8_t *source, size_t source_size,
                                                  uint8_t *scratch, size_t scratch_size)
{
    const uint8_t *sources[] = {source};
    const size_t sizes[] = {source_size};
    return refresh_gc16_sequence(ops, gate, sources, sizes, 1, scratch, scratch_size);
}

read_pico_refresh_result_t read_pico_refresh_gc16_pair(
    const read_pico_refresh_ops_t *ops, read_pico_refresh_gate_t gate, const uint8_t *first_source,
    size_t first_size, const uint8_t *second_source, size_t second_size, uint8_t *scratch,
    size_t scratch_size)
{
    const uint8_t *sources[] = {first_source, second_source};
    const size_t sizes[] = {first_size, second_size};
    return refresh_gc16_sequence(ops, gate, sources, sizes, 2, scratch, scratch_size);
}
