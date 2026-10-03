#include <gtest/gtest.h>

#include <array>
#include <cstring>

#include "read_pico_pmu_codec.h"
#include "read_pico_pmu_protocol.h"

namespace
{
template <size_t N>
void put16(std::array<uint8_t, N> &raw, size_t at, uint16_t value)
{
    raw[at] = value;
    raw[at + 1] = value >> 8;
}
template <size_t N>
void crc(std::array<uint8_t, N> &raw)
{
    put16(raw, N - 2, read_pico_crc16(raw.data(), N - 2));
}
class ReadPicoPmuTest : public testing::Test
{
   protected:
    std::array<uint8_t, 32> id{};
    std::array<uint8_t, 64> status{};
    std::array<uint8_t, 64> response{};
    read_pico_identity_t identity{};
    read_pico_status_t decoded{};
    int mv = 999;

    void SetUp() override
    {
        std::memcpy(id.data(), "PMU1", 4);
        id[4] = 1;
        id[5] = 1;
        id[6] = 1;
        id[8] = 9;
        id[9] = 1;
        put16(id, 10, PMU_DEVICE_ID);
        id[12] = 0x78;
        id[13] = 0x56;
        id[14] = 0x34;
        id[15] = 0x12;
        id[16] = PMU_CAP_BATTERY_ADC | PMU_CAP_CHARGE_STATUS;
        id[20] = 64;
        crc(id);
        ASSERT_TRUE(read_pico_decode_identity(id.data(), id.size(), &identity));
        status[4] = PMU_STATUS_BATTERY_VALID;
        put16(status, 8, 3900);
        put16(status, 12, 753);
        status[16] = PMU_CHARGE_NOT_CHARGING;
        put16(status, 40, 65535);
        crc(status);
        ASSERT_TRUE(read_pico_encode_vcom_get(&identity, 1, response.data(), response.size()));
        response[2] = PMU_FRAME_KIND_RESP;
        response[12] = 4;
        put16(response, 18, 1500);
        response[20] = 1;
        crc(response);
    }
    bool read_status()
    {
        crc(status);
        return read_pico_decode_status(status.data(), status.size(), &identity, &decoded);
    }
    bool read_vcom()
    {
        crc(response);
        return read_pico_decode_vcom(response.data(), response.size(), &identity, 1, &mv);
    }
};

TEST_F(ReadPicoPmuTest, KnownCrcVector)
{
    const char *value = "123456789";
    EXPECT_EQ(read_pico_crc16(reinterpret_cast<const uint8_t *>(value), 9), 0x29b1);
}
TEST_F(ReadPicoPmuTest, IdentityRecordsActualVersionWithoutUid)
{
    EXPECT_EQ(identity.session, 0x12345678u);
    EXPECT_EQ(identity.firmware[2], 9);
}
TEST_F(ReadPicoPmuTest, RejectsCorruptIdentityAndClearsPriorData)
{
    id[6] ^= 1;
    EXPECT_FALSE(read_pico_decode_identity(id.data(), id.size(), &identity));
    EXPECT_EQ(identity.session, 0u);
}
TEST_F(ReadPicoPmuTest, RejectsUnknownProtocolAndDevice)
{
    id[5] = 2;
    crc(id);
    EXPECT_FALSE(read_pico_decode_identity(id.data(), id.size(), &identity));
    id[5] = 1;
    id[10] = 0;
    crc(id);
    EXPECT_FALSE(read_pico_decode_identity(id.data(), id.size(), &identity));
}
TEST_F(ReadPicoPmuTest, RejectsZeroSessionOrWrongFrameSize)
{
    std::memset(id.data() + 12, 0, 4);
    crc(id);
    EXPECT_FALSE(read_pico_decode_identity(id.data(), id.size(), &identity));
    id[12] = 1;
    id[20] = 32;
    crc(id);
    EXPECT_FALSE(read_pico_decode_identity(id.data(), id.size(), &identity));
}
TEST_F(ReadPicoPmuTest, StatusDecodesBatteryAndRecoversSequence)
{
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.battery_mv, 3900);
    EXPECT_EQ(decoded.battery_percent, 75);
    EXPECT_EQ(read_pico_next_sequence(decoded.last_sequence), 1);
    EXPECT_EQ(read_pico_next_sequence(0), 1);
    EXPECT_EQ(read_pico_next_sequence(10), 11);
}
TEST_F(ReadPicoPmuTest, InvalidBatteryFlagOrCapabilityMeansUnknown)
{
    status[4] = 0;
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.battery_mv, -1);
    status[4] = PMU_STATUS_BATTERY_VALID;
    identity.capabilities = 0;
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.battery_percent, -1);
    EXPECT_EQ(decoded.charging, -1);
}
TEST_F(ReadPicoPmuTest, OutOfRangeSocAndUnknownVoltageNotClamped)
{
    put16(status, 8, 65535);
    put16(status, 12, 1001);
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.battery_mv, -1);
    EXPECT_EQ(decoded.battery_percent, -1);
}
TEST_F(ReadPicoPmuTest, CorruptStatusInvalidatesPreviousSample)
{
    ASSERT_TRUE(read_status());
    status[3] ^= 1;
    EXPECT_FALSE(read_pico_decode_status(status.data(), status.size(), &identity, &decoded));
    EXPECT_EQ(decoded.battery_percent, -1);
    EXPECT_EQ(decoded.charging, -1);
}
TEST_F(ReadPicoPmuTest, NotChargingAndFullDoNotProveUsbAbsentOrPresent)
{
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.external_power, READ_PICO_POWER_UNKNOWN);
    status[16] = PMU_CHARGE_FULL_INFERRED;
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.charging, 0);
    EXPECT_EQ(decoded.external_power, READ_PICO_POWER_UNKNOWN);
}
TEST_F(ReadPicoPmuTest, ChargingRequiresConsistentFlag)
{
    status[16] = PMU_CHARGE_CHARGING;
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.charging, -1);
    status[4] |= PMU_STATUS_CHARGING_ACTIVE;
    ASSERT_TRUE(read_status());
    EXPECT_EQ(decoded.charging, 1);
    EXPECT_EQ(decoded.external_power, READ_PICO_POWER_EXTERNAL);
}
TEST_F(ReadPicoPmuTest, UnknownAndFaultChargeStatesStayUnknown)
{
    for (uint8_t value : std::array<uint8_t, 3>{PMU_CHARGE_UNKNOWN, PMU_CHARGE_FAULT, 255}) {
        status[16] = value;
        ASSERT_TRUE(read_status());
        EXPECT_EQ(decoded.charging, -1);
        EXPECT_EQ(decoded.external_power, READ_PICO_POWER_UNKNOWN);
    }
}
TEST_F(ReadPicoPmuTest, VcomRequestOnlyUsesGetOpcode)
{
    std::array<uint8_t, 64> request{};
    ASSERT_TRUE(read_pico_encode_vcom_get(&identity, 1, request.data(), request.size()));
    EXPECT_EQ(request[8], 0x10);
    EXPECT_EQ(request[9], 0x05);
    EXPECT_EQ(request[12], 0);
    EXPECT_EQ(request[14], 0x78);
    EXPECT_EQ(request[2], PMU_FRAME_KIND_REQ);
    EXPECT_FALSE(read_pico_encode_vcom_get(&identity, 0, request.data(), request.size()));
}
TEST_F(ReadPicoPmuTest, ValidFactoryVcom)
{
    ASSERT_TRUE(read_vcom());
    EXPECT_EQ(mv, 1500);
}
TEST_F(ReadPicoPmuTest, MissingCalibrationClearsOldValue)
{
    response[20] = 0;
    EXPECT_FALSE(read_vcom());
    EXPECT_EQ(mv, -1);
}
TEST_F(ReadPicoPmuTest, VcomBoundsAndAlignment)
{
    for (uint16_t value : {0, 490, 501, 2510, 65535}) {
        put16(response, 18, value);
        EXPECT_FALSE(read_vcom());
        EXPECT_EQ(mv, -1);
    }
    for (uint16_t value : {500, 2500}) {
        put16(response, 18, value);
        ASSERT_TRUE(read_vcom());
        EXPECT_EQ(mv, value);
    }
}
TEST_F(ReadPicoPmuTest, RejectsMismatchedResponseFields)
{
    const auto original = response;
    for (size_t index : {0u, 1u, 2u, 4u, 5u, 6u, 8u, 10u, 12u, 14u}) {
        response = original;
        response[index] ^= 1;
        EXPECT_FALSE(read_vcom()) << index;
    }
}
TEST_F(ReadPicoPmuTest, RejectsCorruptAndTruncatedResponse)
{
    response[18] ^= 1;
    EXPECT_FALSE(read_pico_decode_vcom(response.data(), response.size(), &identity, 1, &mv));
    EXPECT_FALSE(read_pico_decode_vcom(response.data(), 63, &identity, 1, &mv));
    EXPECT_EQ(mv, -1);
}
TEST_F(ReadPicoPmuTest, RejectsNullInputs)
{
    EXPECT_FALSE(read_pico_decode_identity(nullptr, 32, &identity));
    EXPECT_FALSE(read_pico_decode_status(nullptr, 64, &identity, &decoded));
    EXPECT_FALSE(read_pico_decode_vcom(nullptr, 64, &identity, 1, &mv));
    EXPECT_FALSE(read_pico_decode_identity(id.data(), 32, nullptr));
}
}  // namespace
