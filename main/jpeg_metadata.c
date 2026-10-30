#include "jpeg_metadata.h"

#include <string.h>

static uint16_t u16(const uint8_t *p, bool little)
{
    return little ? (uint16_t) (p[0] | p[1] << 8) : (uint16_t) (p[0] << 8 | p[1]);
}

static uint32_t u32(const uint8_t *p, bool little)
{
    return little ? (uint32_t) p[0] | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 |
                        (uint32_t) p[3] << 24
                  : (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 | (uint32_t) p[2] << 8 |
                        (uint32_t) p[3];
}

static bool read_at(const jpeg_metadata_io_t *io, size_t offset, uint8_t *p, size_t n)
{
    return offset <= io->size && n <= io->size - offset && io->read_at(io->context, offset, p, n);
}

static jpeg_metadata_error_t exif(const jpeg_metadata_io_t *io, size_t base, size_t size,
                                  unsigned *orientation, bool *seen)
{
    uint8_t p[12];
    if (size < 8)
        return JPEG_METADATA_INVALID;
    if (!read_at(io, base, p, 8))
        return JPEG_METADATA_READ_FAILED;
    bool little = p[0] == 'I' && p[1] == 'I';
    if ((!little && !(p[0] == 'M' && p[1] == 'M')) || u16(p + 2, little) != 42)
        return JPEG_METADATA_INVALID;
    size_t offset = u32(p + 4, little);
    if (offset < 8 || offset > size - 2)
        return JPEG_METADATA_INVALID;
    if (!read_at(io, base + offset, p, 2))
        return JPEG_METADATA_READ_FAILED;
    unsigned entries = u16(p, little);
    offset += 2;
    if (entries > 4096)
        return JPEG_METADATA_LIMIT;
    if (size - offset < 4 || entries > (size - offset - 4) / 12)
        return JPEG_METADATA_INVALID;
    for (unsigned i = 0; i < entries; ++i) {
        if (!read_at(io, base + offset + i * 12, p, 12))
            return JPEG_METADATA_READ_FAILED;
        if (u16(p, little) != 0x0112)
            continue;
        unsigned value = u16(p + 8, little);
        if (*seen || u16(p + 2, little) != 3 || u32(p + 4, little) != 1 || value < 1 || value > 8)
            return JPEG_METADATA_INVALID;
        *seen = true;
        *orientation = value;
    }
    return JPEG_METADATA_OK;
}

jpeg_metadata_error_t jpeg_metadata_read(const jpeg_metadata_io_t *io, jpeg_metadata_t *out)
{
    if (!io || !io->read_at || !out || io->size < 4)
        return JPEG_METADATA_INVALID;
    *out = (jpeg_metadata_t) {.orientation = 1};
    if (io->size > 32u * 1024u * 1024u)
        return JPEG_METADATA_LIMIT;
    uint8_t p[9];
    if (!read_at(io, 0, p, 2))
        return JPEG_METADATA_READ_FAILED;
    if (p[0] != 0xff || p[1] != 0xd8)
        return JPEG_METADATA_INVALID;
    size_t offset = 2;
    bool seen = false, sof = false;
    for (unsigned markers = 0; markers < 4096; ++markers) {
        if (offset >= 4u * 1024u * 1024u)
            return JPEG_METADATA_LIMIT;
        if (!read_at(io, offset++, p, 1))
            return JPEG_METADATA_READ_FAILED;
        if (p[0] != 0xff)
            return JPEG_METADATA_INVALID;
        do {
            if (offset >= 4u * 1024u * 1024u)
                return JPEG_METADATA_LIMIT;
            if (!read_at(io, offset++, p, 1))
                return JPEG_METADATA_READ_FAILED;
        } while (p[0] == 0xff);
        unsigned marker = p[0];
        if (marker == 0 || marker == 0xd8 || marker == 0xd9 || (marker >= 0xd0 && marker <= 0xd7))
            return JPEG_METADATA_INVALID;
        if (marker == 0x01)
            continue;
        if (!read_at(io, offset, p, 2))
            return JPEG_METADATA_READ_FAILED;
        size_t length = u16(p, false);
        if (length < 2 || length > io->size - offset)
            return JPEG_METADATA_INVALID;
        size_t base = offset + 2, size = length - 2;
        if (offset + length > 4u * 1024u * 1024u)
            return JPEG_METADATA_LIMIT;
        if (marker == 0xe1 && size >= 6) {
            if (!read_at(io, base, p, 6))
                return JPEG_METADATA_READ_FAILED;
            if (memcmp(p, "Exif\0\0", 6) == 0) {
                jpeg_metadata_error_t err = exif(io, base + 6, size - 6, &out->orientation, &seen);
                if (err != JPEG_METADATA_OK)
                    return err;
            }
        }
        if (marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 &&
            marker != 0xcc) {
            if (marker != 0xc0)
                return JPEG_METADATA_UNSUPPORTED;
            if (sof || size < 6)
                return JPEG_METADATA_INVALID;
            if (!read_at(io, base, p, 6))
                return JPEG_METADATA_READ_FAILED;
            if (p[0] != 8 || (p[5] != 1 && p[5] != 3))
                return JPEG_METADATA_UNSUPPORTED;
            if (size != 6u + 3u * p[5])
                return JPEG_METADATA_INVALID;
            out->height = u16(p + 1, false);
            out->width = u16(p + 3, false);
            if (!out->width || !out->height || out->width > 16384 || out->height > 16384 ||
                (uint64_t) out->width * out->height > UINT64_C(64000000))
                return JPEG_METADATA_LIMIT;
            sof = true;
        }
        if (marker == 0xda) {
            if (!sof || size < 4)
                return JPEG_METADATA_INVALID;
            return JPEG_METADATA_OK;
        }
        offset += length;
    }
    return JPEG_METADATA_LIMIT;
}

bool jpeg_metadata_scale(const jpeg_metadata_t *info, size_t budget, unsigned *scale,
                         size_t *rgb_bytes)
{
    if (!info || !scale || !rgb_bytes || !info->width || !info->height || info->width > 16384 ||
        info->height > 16384 || (uint64_t) info->width * info->height > UINT64_C(64000000) ||
        info->orientation < 1 || info->orientation > 8)
        return false;
    for (unsigned s = 0; s <= 3; ++s) {
        unsigned w = info->width >> s, h = info->height >> s;
        size_t n = (size_t) w * h * 3;
        if (w && h && n <= budget) {
            *scale = s;
            *rgb_bytes = n;
            return true;
        }
    }
    return false;
}

bool jpeg_orientation_map(unsigned w, unsigned h, unsigned orientation, unsigned x, unsigned y,
                          unsigned *sx, unsigned *sy)
{
    if (!sx || !sy || !w || !h || orientation < 1 || orientation > 8 ||
        x >= (orientation >= 5 ? h : w) || y >= (orientation >= 5 ? w : h))
        return false;
    switch (orientation) {
    case 1:
        *sx = x;
        *sy = y;
        break;
    case 2:
        *sx = w - 1 - x;
        *sy = y;
        break;
    case 3:
        *sx = w - 1 - x;
        *sy = h - 1 - y;
        break;
    case 4:
        *sx = x;
        *sy = h - 1 - y;
        break;
    case 5:
        *sx = y;
        *sy = x;
        break;
    case 6:
        *sx = y;
        *sy = h - 1 - x;
        break;
    case 7:
        *sx = w - 1 - y;
        *sy = h - 1 - x;
        break;
    case 8:
        *sx = w - 1 - y;
        *sy = x;
        break;
    }
    return true;
}
