#include "read_pico_pmu_codec.h"

#include <string.h>

#include "read_pico_pmu_protocol.h"

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) |
           ((uint32_t) p[3] << 24);
}

static void wr16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t) value;
    p[1] = (uint8_t) (value >> 8);
}

uint16_t read_pico_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xffff;
    while (length--) {
        crc ^= (uint16_t) *data++ << 8;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (uint16_t) ((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
        }
    }
    return crc;
}

static bool valid_crc(const uint8_t *raw, size_t length)
{
    return read_pico_crc16(raw, length - 2) == rd16(raw + length - 2);
}

bool read_pico_decode_identity(const uint8_t *raw, size_t length, read_pico_identity_t *out)
{
    if (out == NULL)
        return false;
    memset(out, 0, sizeof(*out));
    if (raw == NULL || length != PMU_IDENTITY_SIZE || !valid_crc(raw, length) ||
        memcmp(raw, "PMU1", 4) != 0 || raw[4] != PMU_PROTOCOL_MAJOR ||
        raw[5] != PMU_PROTOCOL_MINOR || rd16(raw + 10) != PMU_DEVICE_ID ||
        raw[20] != PMU_FRAME_SIZE || rd32(raw + 12) == 0) {
        return false;
    }
    memcpy(out->firmware, raw + 6, sizeof(out->firmware));
    out->hardware = raw[9];
    out->session = rd32(raw + 12);
    out->capabilities = rd32(raw + 16);
    return true;
}

bool read_pico_decode_status(const uint8_t *raw, size_t length,
                             const read_pico_identity_t *identity, read_pico_status_t *out)
{
    if (out == NULL)
        return false;
    memset(out, 0, sizeof(*out));
    out->battery_mv = -1;
    out->battery_percent = -1;
    out->charging = -1;
    if (raw == NULL || length != PMU_STATUS_SIZE || identity == NULL || identity->session == 0 ||
        !valid_crc(raw, length)) {
        return false;
    }
    uint32_t flags = rd32(raw + 4);
    if ((identity->capabilities & PMU_CAP_BATTERY_ADC) && (flags & PMU_STATUS_BATTERY_VALID)) {
        uint16_t mv = rd16(raw + 8);
        if (mv != 0 && mv != UINT16_MAX)
            out->battery_mv = mv;
        uint16_t soc = rd16(raw + 12);
        if (soc <= 1000)
            out->battery_percent = soc / 10;
    }
    if (identity->capabilities & PMU_CAP_CHARGE_STATUS) {
        if (raw[16] == PMU_CHARGE_CHARGING && (flags & PMU_STATUS_CHARGING_ACTIVE)) {
            out->charging = 1;
            out->external_power = READ_PICO_POWER_EXTERNAL;
        } else if (raw[16] == PMU_CHARGE_FULL_INFERRED && !(flags & PMU_STATUS_CHARGING_ACTIVE)) {
            out->charging = 0;
            // FULL is inferred, not a verified VBUS detector. Keep external power unknown.
        } else if (raw[16] == PMU_CHARGE_NOT_CHARGING && !(flags & PMU_STATUS_CHARGING_ACTIVE)) {
            out->charging = 0;
        }
    }
    out->power_state = raw[2];
    out->fault_flags = rd32(raw + 20);
    out->last_sequence = rd16(raw + 40);
    return true;
}

uint16_t read_pico_next_sequence(uint16_t previous)
{
    return previous == UINT16_MAX ? 1 : previous + 1;
}

bool read_pico_encode_vcom_get(const read_pico_identity_t *identity, uint16_t sequence,
                               uint8_t *frame, size_t length)
{
    if (frame == NULL || length != PMU_FRAME_SIZE)
        return false;
    memset(frame, 0, length);
    if (identity == NULL || identity->session == 0 || sequence == 0)
        return false;
    frame[0] = PMU_FRAME_MAGIC;
    frame[1] = 1;
    frame[2] = PMU_FRAME_KIND_REQ;
    frame[4] = PMU_PROTOCOL_MAJOR;
    frame[5] = PMU_PROTOCOL_MINOR;
    wr16(frame + 6, sequence);
    wr16(frame + 8, PMU_CMD_VCOM_GET);
    for (int byte = 0; byte < 4; ++byte)
        frame[14 + byte] = identity->session >> (byte * 8);
    wr16(frame + 62, read_pico_crc16(frame, 62));
    return true;
}

bool read_pico_decode_vcom(const uint8_t *frame, size_t length,
                           const read_pico_identity_t *identity, uint16_t sequence, int *mv)
{
    if (mv == NULL)
        return false;
    *mv = -1;
    if (frame == NULL || length != PMU_FRAME_SIZE || identity == NULL || identity->session == 0 ||
        sequence == 0 || !valid_crc(frame, length) || frame[0] != PMU_FRAME_MAGIC ||
        frame[1] != 1 || frame[2] != PMU_FRAME_KIND_RESP || frame[4] != PMU_PROTOCOL_MAJOR ||
        frame[5] != PMU_PROTOCOL_MINOR || rd16(frame + 6) != sequence ||
        rd16(frame + 8) != PMU_CMD_VCOM_GET || rd16(frame + 10) != PMU_STATUS_OK ||
        frame[12] != 4 || rd32(frame + 14) != identity->session || frame[20] != 1) {
        return false;
    }
    int value = rd16(frame + 18);
    if (value < 500 || value > 2500 || value % 10 != 0)
        return false;
    *mv = value;
    return true;
}
