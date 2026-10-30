#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "read_pico_pmu_command.h"
#include "read_pico_pmu_protocol.h"
}

namespace
{
read_pico_identity_t identity()
{
    read_pico_identity_t id{};
    id.session = 0x11223344;
    return id;
}

void finish(uint8_t *frame)
{
    uint16_t crc = read_pico_crc16(frame, 62);
    frame[62] = crc & 0xff;
    frame[63] = crc >> 8;
}

// Turn an encoded request into the PMU's response frame.
void respond(uint8_t *frame, uint16_t status, const uint8_t *payload, uint8_t length)
{
    frame[2] = PMU_FRAME_KIND_RESP;
    frame[10] = status & 0xff;
    frame[11] = status >> 8;
    frame[12] = length;
    std::memset(frame + 18, 0, 44);
    if (length) std::memcpy(frame + 18, payload, length);
    finish(frame);
}
}  // namespace

TEST(ReadPicoPmuCommand, EncodesDocumentedRequestLayout)
{
    auto id = identity();
    uint8_t payload[1] = {0};
    uint8_t frame[64];
    ASSERT_TRUE(read_pico_pmu_encode_request(&id, 0x0102, PMU_CMD_HOST_READY, payload, 1, frame, sizeof(frame)));
    EXPECT_EQ(frame[0], 0xA5);
    EXPECT_EQ(frame[1], 1);
    EXPECT_EQ(frame[2], PMU_FRAME_KIND_REQ);
    EXPECT_EQ(frame[4], PMU_PROTOCOL_MAJOR);
    EXPECT_EQ(frame[5], PMU_PROTOCOL_MINOR);
    EXPECT_EQ(frame[6] | frame[7] << 8, 0x0102);
    EXPECT_EQ(frame[8] | frame[9] << 8, PMU_CMD_HOST_READY);
    EXPECT_EQ(frame[10] | frame[11], 0);
    EXPECT_EQ(frame[12], 1);
    EXPECT_EQ(frame[14] | frame[15] << 8 | frame[16] << 16 | (uint32_t) frame[17] << 24, 0x11223344u);
    EXPECT_EQ(read_pico_crc16(frame, 62), frame[62] | frame[63] << 8);
}

TEST(ReadPicoPmuCommand, RefusesBadEncodeArguments)
{
    auto id = identity();
    uint8_t frame[64];
    uint8_t big[READ_PICO_PMU_PAYLOAD_MAX + 1] = {};
    EXPECT_FALSE(read_pico_pmu_encode_request(&id, 0, PMU_CMD_PING, nullptr, 0, frame, sizeof(frame)));
    EXPECT_FALSE(read_pico_pmu_encode_request(&id, 1, PMU_CMD_PING, big, sizeof(big), frame, sizeof(frame)));
    EXPECT_FALSE(read_pico_pmu_encode_request(&id, 1, PMU_CMD_PING, nullptr, 3, frame, sizeof(frame)));
    EXPECT_FALSE(read_pico_pmu_encode_request(&id, 1, PMU_CMD_PING, nullptr, 0, frame, 63));
    read_pico_identity_t none{};
    EXPECT_FALSE(read_pico_pmu_encode_request(&none, 1, PMU_CMD_PING, nullptr, 0, frame, sizeof(frame)));
    for (uint8_t b : frame) ASSERT_EQ(b, 0);  // zeroed on refusal
}

TEST(ReadPicoPmuCommand, DecodesMatchingResponseOnly)
{
    auto id = identity();
    uint8_t frame[64];
    ASSERT_TRUE(read_pico_pmu_encode_request(&id, 7, PMU_CMD_ALARM_GET, nullptr, 0, frame, sizeof(frame)));
    const uint8_t reply[10] = {1, 0, 0x3c, 0, 0, 0, 1, 2, 3, 4};
    respond(frame, PMU_STATUS_OK, reply, sizeof(reply));
    uint16_t status;
    uint8_t out[4];
    size_t length;
    ASSERT_TRUE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 7, PMU_CMD_ALARM_GET, &status, out,
                                              sizeof(out), &length));
    EXPECT_EQ(status, PMU_STATUS_OK);
    EXPECT_EQ(length, 10u);  // full length reported, copy truncated to capacity
    EXPECT_EQ(out[2], 0x3c);
    EXPECT_FALSE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 8, PMU_CMD_ALARM_GET, &status,
                                               nullptr, 0, nullptr));  // other sequence
    EXPECT_FALSE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 7, PMU_CMD_PING, &status, nullptr, 0,
                                               nullptr));  // other opcode
    read_pico_identity_t other{};
    other.session = 9;
    EXPECT_FALSE(read_pico_pmu_decode_response(frame, sizeof(frame), &other, 7, PMU_CMD_ALARM_GET, &status,
                                               nullptr, 0, nullptr));
    frame[20] ^= 1;  // payload corruption breaks the CRC
    EXPECT_FALSE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 7, PMU_CMD_ALARM_GET, &status,
                                               nullptr, 0, nullptr));
    EXPECT_EQ(status, 0xffff);
    frame[20] ^= 1;
    frame[2] = PMU_FRAME_KIND_REQ;  // a request echo is not a response
    finish(frame);
    EXPECT_FALSE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 7, PMU_CMD_ALARM_GET, &status,
                                               nullptr, 0, nullptr));
}

TEST(ReadPicoPmuCommand, ErrorStatusIsReportedNotHidden)
{
    auto id = identity();
    uint8_t frame[64];
    ASSERT_TRUE(read_pico_pmu_encode_request(&id, 3, PMU_CMD_HOST_SOFT_SLEEP, nullptr, 0, frame, sizeof(frame)));
    respond(frame, PMU_STATUS_INVALID_STATE, nullptr, 0);
    uint16_t status;
    ASSERT_TRUE(read_pico_pmu_decode_response(frame, sizeof(frame), &id, 3, PMU_CMD_HOST_SOFT_SLEEP, &status,
                                              nullptr, 0, nullptr));
    EXPECT_EQ(status, PMU_STATUS_INVALID_STATE);
}

TEST(ReadPicoPmuCommand, PayloadBuildersEnforceDocumentedRanges)
{
    uint8_t out[8];
    EXPECT_EQ(read_pico_pmu_time_sync_payload(READ_PICO_PMU_TIME_MIN - 1, out), 0u);
    EXPECT_EQ(read_pico_pmu_time_sync_payload(READ_PICO_PMU_TIME_MAX + 1, out), 0u);
    ASSERT_EQ(read_pico_pmu_time_sync_payload(1793200684u, out), 4u);
    EXPECT_EQ(out[0] | out[1] << 8 | out[2] << 16 | (uint32_t) out[3] << 24, 1793200684u);
    EXPECT_EQ(read_pico_pmu_alarm_once_payload(9, out), 0u);
    EXPECT_EQ(read_pico_pmu_alarm_once_payload(READ_PICO_PMU_ALARM_MAX_S + 1, out), 0u);
    ASSERT_EQ(read_pico_pmu_alarm_once_payload(43200, out), 5u);
    EXPECT_EQ(out[0], 1);
    EXPECT_EQ(out[1] | out[2] << 8 | out[3] << 16 | (uint32_t) out[4] << 24, 43200u);
    EXPECT_EQ(read_pico_pmu_alarm_once_payload(60, nullptr), 0u);
}
