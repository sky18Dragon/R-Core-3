#include "read_pico_pmu_command.h"

#include <string.h>

#include "read_pico_pmu_protocol.h"

#define FRAME_SEQUENCE 6
#define FRAME_OPCODE 8
#define FRAME_STATUS 10
#define FRAME_LENGTH 12
#define FRAME_SESSION 14
#define FRAME_PAYLOAD 18
#define FRAME_CRC 62

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

static void wr32(uint8_t *p, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        p[i] = (uint8_t) (value >> (8 * i));
}

bool read_pico_pmu_encode_request(const read_pico_identity_t *identity, uint16_t sequence,
                                  uint16_t opcode, const uint8_t *payload, size_t payload_length,
                                  uint8_t *frame, size_t frame_length)
{
    if (frame == NULL || frame_length != PMU_FRAME_SIZE)
        return false;
    memset(frame, 0, frame_length);
    if (identity == NULL || identity->session == 0 || sequence == 0 ||
        payload_length > READ_PICO_PMU_PAYLOAD_MAX || (payload_length && payload == NULL))
        return false;
    frame[0] = PMU_FRAME_MAGIC;
    frame[1] = 1;
    frame[2] = PMU_FRAME_KIND_REQ;
    frame[4] = PMU_PROTOCOL_MAJOR;
    frame[5] = PMU_PROTOCOL_MINOR;
    wr16(frame + FRAME_SEQUENCE, sequence);
    wr16(frame + FRAME_OPCODE, opcode);
    frame[FRAME_LENGTH] = (uint8_t) payload_length;
    wr32(frame + FRAME_SESSION, identity->session);
    if (payload_length)
        memcpy(frame + FRAME_PAYLOAD, payload, payload_length);
    wr16(frame + FRAME_CRC, read_pico_crc16(frame, FRAME_CRC));
    return true;
}

bool read_pico_pmu_decode_response(const uint8_t *frame, size_t frame_length,
                                   const read_pico_identity_t *identity, uint16_t sequence,
                                   uint16_t opcode, uint16_t *status, uint8_t *payload,
                                   size_t capacity, size_t *payload_length)
{
    if (status) *status = 0xffff;
    if (payload_length) *payload_length = 0;
    if (frame == NULL || frame_length != PMU_FRAME_SIZE || identity == NULL ||
        identity->session == 0 || sequence == 0 ||
        read_pico_crc16(frame, FRAME_CRC) != rd16(frame + FRAME_CRC) || frame[0] != PMU_FRAME_MAGIC ||
        frame[1] != 1 || frame[2] != PMU_FRAME_KIND_RESP || frame[4] != PMU_PROTOCOL_MAJOR ||
        frame[5] != PMU_PROTOCOL_MINOR || rd16(frame + FRAME_SEQUENCE) != sequence ||
        rd16(frame + FRAME_OPCODE) != opcode || rd32(frame + FRAME_SESSION) != identity->session ||
        frame[FRAME_LENGTH] > READ_PICO_PMU_PAYLOAD_MAX)
        return false;
    if (status) *status = rd16(frame + FRAME_STATUS);
    size_t length = frame[FRAME_LENGTH];
    if (payload) {
        size_t n = length < capacity ? length : capacity;
        memcpy(payload, frame + FRAME_PAYLOAD, n);
    }
    if (payload_length) *payload_length = length;
    return true;
}

size_t read_pico_pmu_time_sync_payload(uint32_t unix_seconds, uint8_t *out)
{
    if (!out || unix_seconds < READ_PICO_PMU_TIME_MIN || unix_seconds > READ_PICO_PMU_TIME_MAX)
        return 0;
    wr32(out, unix_seconds);
    return 4;
}

size_t read_pico_pmu_alarm_once_payload(uint32_t seconds, uint8_t *out)
{
    if (!out || seconds < READ_PICO_PMU_ALARM_MIN_S || seconds > READ_PICO_PMU_ALARM_MAX_S)
        return 0;
    out[0] = 1;  // once
    wr32(out + 1, seconds);
    return 5;
}
