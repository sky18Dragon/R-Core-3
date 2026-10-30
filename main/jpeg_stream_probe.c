#include "jpeg_stream_probe.h"

#include <string.h>

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#endif
#if defined(CONFIG_JD_USE_ROM) && CONFIG_JD_USE_ROM
#include "rom/tjpgd.h"
typedef UINT input_size_t;
typedef UINT output_result_t;
#else
#include "tjpgd.h"
typedef size_t input_size_t;
typedef int output_result_t;
#endif

#if JD_FORMAT != 0 || JD_USE_SCALE != 1
#error "JPEG probe requires RGB888 and decoder scaling"
#endif

typedef struct {
    const jpeg_stream_probe_io_t *io;
    jpeg_stream_probe_result_t result;
    uint8_t *rgb;
} stream_t;

static input_size_t stream_read(JDEC *decoder, uint8_t *data, input_size_t requested)
{
    stream_t *stream = decoder->device;
    uint8_t discard[256];
    size_t total = 0;
    while (total < requested && stream->result.error == JPEG_STREAM_OK) {
        if (!stream->io->media_ready(stream->io->context)) {
            stream->result.error = JPEG_STREAM_MEDIA_FAILED;
            break;
        }
        size_t size = requested - total;
        if (size > sizeof(discard))
            size = sizeof(discard);
        int got = stream->io->read(stream->io->context, data ? data + total : discard, size);
        if (got < 0 || (size_t) got > size) {
            stream->result.error = JPEG_STREAM_READ_FAILED;
            break;
        }
        if (got == 0)
            break;
        total += (size_t) got;
        stream->result.bytes_read += (size_t) got;
    }
    return (input_size_t) total;
}

static output_result_t stream_output(JDEC *decoder, void *bitmap, JRECT *rect)
{
    stream_t *stream = decoder->device;
    jpeg_stream_probe_result_t *r = &stream->result;
    if (r->error != JPEG_STREAM_OK)
        return 0;
    if (!bitmap || !rect || rect->left > rect->right || rect->top > rect->bottom ||
        rect->right >= r->output_width || rect->bottom >= r->output_height) {
        r->error = JPEG_STREAM_DIMENSIONS_FAILED;
        return 0;
    }
    size_t pixels = (size_t) (rect->right - rect->left + 1) * (rect->bottom - rect->top + 1);
    size_t expected = (size_t) r->output_width * r->output_height;
    if (pixels > expected - r->pixels) {
        r->error = JPEG_STREAM_DIMENSIONS_FAILED;
        return 0;
    }
    const uint8_t *bytes = bitmap;
    if (stream->rgb) {
        size_t stride = (size_t) r->output_width * 3;
        size_t row_size = (size_t) (rect->right - rect->left + 1) * 3;
        for (unsigned y = rect->top; y <= rect->bottom; ++y) {
            memcpy(stream->rgb + y * stride + rect->left * 3, bytes, row_size);
            bytes += row_size;
        }
    } else
        for (size_t i = 0; i < pixels * 3; ++i) {
            r->rgb_crc32 ^= bytes[i];
            for (unsigned bit = 0; bit < 8; ++bit)
                r->rgb_crc32 =
                    (r->rgb_crc32 >> 1) ^ ((r->rgb_crc32 & 1) ? UINT32_C(0xedb88320) : 0);
        }
    r->pixels += pixels;
    ++r->blocks;
    return 1;
}

static jpeg_stream_probe_result_t decode(const jpeg_stream_probe_io_t *io, void *work,
                                         size_t work_size, unsigned scale, uint8_t *rgb,
                                         size_t rgb_size)
{
    stream_t stream = {.io = io, .result = {.error = JPEG_STREAM_INVALID}, .rgb = rgb};
    if (!io || !io->read || !io->media_ready || !work || work_size < 3100 ||
        work_size > UINT32_MAX || scale > 3)
        return stream.result;
    stream.result.error = JPEG_STREAM_OK;
    JDEC decoder = {0};
    JRESULT result = jd_prepare(&decoder, stream_read, work, work_size, &stream);
    stream.result.decoder_result = result;
    if (result != JDR_OK) {
        if (stream.result.error == JPEG_STREAM_OK)
            stream.result.error = JPEG_STREAM_PREPARE_FAILED;
        return stream.result;
    }
    stream.result.width = decoder.width;
    stream.result.height = decoder.height;
    stream.result.output_width = decoder.width >> scale;
    stream.result.output_height = decoder.height >> scale;
    // Bound work/CPU before starting full decode. This covers current 25MP samples.
    if (!stream.result.output_width || !stream.result.output_height || decoder.width > 16384 ||
        decoder.height > 16384 || (uint64_t) decoder.width * decoder.height > UINT64_C(64000000) ||
        (rgb && rgb_size < (size_t) stream.result.output_width * stream.result.output_height * 3)) {
        stream.result.error = JPEG_STREAM_DIMENSIONS_FAILED;
        return stream.result;
    }
    stream.result.rgb_crc32 = rgb ? 0 : UINT32_MAX;
    result = jd_decomp(&decoder, stream_output, (uint8_t) scale);
    stream.result.decoder_result = result;
    if (!rgb)
        stream.result.rgb_crc32 ^= UINT32_MAX;
    if (stream.result.error == JPEG_STREAM_OK && !io->media_ready(io->context))
        stream.result.error = JPEG_STREAM_MEDIA_FAILED;
    if (stream.result.error == JPEG_STREAM_OK &&
        (result != JDR_OK ||
         stream.result.pixels != (size_t) stream.result.output_width * stream.result.output_height))
        stream.result.error = JPEG_STREAM_DECODE_FAILED;
    return stream.result;
}

jpeg_stream_probe_result_t jpeg_stream_probe_run(const jpeg_stream_probe_io_t *io, void *work,
                                                 size_t work_size, unsigned scale)
{
    return decode(io, work, work_size, scale, NULL, 0);
}

jpeg_stream_probe_result_t jpeg_stream_decode_rgb(const jpeg_stream_probe_io_t *io, void *work,
                                                  size_t work_size, unsigned scale, uint8_t *rgb,
                                                  size_t rgb_size)
{
    if (!rgb)
        return (jpeg_stream_probe_result_t) {.error = JPEG_STREAM_INVALID};
    return decode(io, work, work_size, scale, rgb, rgb_size);
}

const char *jpeg_stream_probe_error_name(jpeg_stream_probe_error_t error)
{
    switch (error) {
    case JPEG_STREAM_OK:
        return "OK";
    case JPEG_STREAM_INVALID:
        return "INVALID";
    case JPEG_STREAM_MEDIA_FAILED:
        return "MEDIA_FAILED";
    case JPEG_STREAM_READ_FAILED:
        return "READ_FAILED";
    case JPEG_STREAM_PREPARE_FAILED:
        return "PREPARE_FAILED";
    case JPEG_STREAM_DIMENSIONS_FAILED:
        return "DIMENSIONS_FAILED";
    case JPEG_STREAM_DECODE_FAILED:
        return "DECODE_FAILED";
    }
    return "UNKNOWN";
}
