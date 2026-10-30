#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <mutex>
#include <thread>

#include "board_hal.h"
#include "driver/i2c_master.h"
#include "freertos/semphr.h"
extern "C" {
#include "read_pico_board.h"
}
#include "read_pico_pmu_protocol.h"

struct fake_bus {
};
struct fake_device {
    uint16_t address;
};

namespace
{
fake_bus bus_instance;
int bus_count;
int device_count;
int calls;
int fail_at;
int commands;
int identity_reads;
int status_reads;
int corrupt_status_reads;
bool fail_after_torn_status;
bool simulate_pmu_reboot;
bool no_vcom;
bool corrupt_status;
bool cleanup_failure;
int64_t now;
std::recursive_mutex board_mutex;
std::mutex owner_mutex;
thread_local int task_identity;
void *mutex_owner;
int mutex_depth;
uint8_t ioe_output;
uint8_t ioe_config;
uint8_t ioe_input;
bool corrupt_ioe_readback;
std::array<uint8_t, 65> request;

void put16(uint8_t *p, uint16_t value)
{
    p[0] = value;
    p[1] = value >> 8;
}
esp_err_t io_result()
{
    return ++calls == fail_at ? ESP_ERR_TIMEOUT : ESP_OK;
}
}  // namespace

extern "C" {
SemaphoreHandle_t xSemaphoreCreateRecursiveMutexStatic(StaticSemaphore_t *)
{
    return &board_mutex;
}
int xSemaphoreTakeRecursive(SemaphoreHandle_t, TickType_t)
{
    if (!board_mutex.try_lock())
        return 0;
    std::lock_guard<std::mutex> guard(owner_mutex);
    mutex_owner = &task_identity;
    ++mutex_depth;
    return 1;
}
int xSemaphoreGiveRecursive(SemaphoreHandle_t)
{
    {
        std::lock_guard<std::mutex> guard(owner_mutex);
        EXPECT_EQ(mutex_owner, &task_identity);
        if (--mutex_depth == 0)
            mutex_owner = nullptr;
    }
    board_mutex.unlock();
    return 1;
}
TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t)
{
    std::lock_guard<std::mutex> guard(owner_mutex);
    return mutex_owner;
}
TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return &task_identity;
}
const char *esp_err_to_name(esp_err_t)
{
    return "mock_error";
}
int64_t esp_timer_get_time(void)
{
    return now;
}
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *config, i2c_master_bus_handle_t *out)
{
    EXPECT_EQ(config->sda_io_num, 39);
    EXPECT_EQ(config->scl_io_num, 40);
    if (io_result() != ESP_OK)
        return ESP_ERR_TIMEOUT;
    ++bus_count;
    *out = &bus_instance;
    return ESP_OK;
}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t)
{
    EXPECT_EQ(device_count, 0);
    --bus_count;
    return ESP_OK;
}
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t, const i2c_device_config_t *config,
                                    i2c_master_dev_handle_t *out)
{
    if (io_result() != ESP_OK)
        return ESP_ERR_TIMEOUT;
    EXPECT_EQ(config->scl_speed_hz, 400000u);
    ++device_count;
    *out = new fake_device{config->device_address};
    return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t device)
{
    if (cleanup_failure)
        return ESP_ERR_TIMEOUT;
    delete device;
    --device_count;
    return ESP_OK;
}
esp_err_t i2c_master_probe(i2c_master_bus_handle_t, uint16_t address, int)
{
    EXPECT_EQ(address, 0x62);
    ++calls;
    return ESP_ERR_NOT_FOUND;  // Rails off is a normal result.
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t device, const uint8_t *data, size_t length,
                              int)
{
    if (device->address == 0x24) {
        EXPECT_EQ(length, 2u);
        EXPECT_TRUE(data[0] == 2 || data[0] == 6);
        esp_err_t err = io_result();
        if (err == ESP_OK) {
            if (data[0] == 2)
                ioe_output = data[1];
            else
                ioe_config = data[1];
        }
        return err;
    }
    EXPECT_EQ(device->address, PMU_I2C_ADDR);
    EXPECT_EQ(length, request.size());
    if (length != request.size())
        return ESP_ERR_INVALID_SIZE;
    EXPECT_EQ(data[0], PMU_REG_COMMAND);
    EXPECT_EQ(data[9], 0x10);
    EXPECT_EQ(data[10], 0x05);
    EXPECT_EQ(data[13], 0);
    ++commands;
    std::memcpy(request.data(), data, length);
    return io_result();
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t device, const uint8_t *reg,
                                      size_t pointer_length, uint8_t *raw, size_t length, int)
{
    EXPECT_EQ(pointer_length, 1u);
    if (io_result() != ESP_OK)
        return ESP_ERR_TIMEOUT;
    std::memset(raw, 0, length);
    if (device->address == 0x24) {
        EXPECT_TRUE(*reg == 6 || *reg == 2 || *reg == 0);
        EXPECT_EQ(length, 1u);
        raw[0] = *reg == 6 ? ioe_config : *reg == 2 ? ioe_output : ioe_input;
        if (corrupt_ioe_readback)
            raw[0] ^= 1;
        return ESP_OK;
    }
    EXPECT_EQ(device->address, PMU_I2C_ADDR);
    if (*reg == PMU_REG_IDENTITY) {
        EXPECT_EQ(length, 32u);
        std::memcpy(raw, "PMU1", 4);
        raw[4] = raw[5] = raw[6] = 1;
        raw[8] = 9;
        raw[9] = 1;
        put16(raw + 10, PMU_DEVICE_ID);
        raw[12] = (++identity_reads > 1 && simulate_pmu_reboot) ? 2 : 1;
        raw[16] = PMU_CAP_BATTERY_ADC | PMU_CAP_CHARGE_STATUS;
        raw[20] = 64;
    } else if (*reg == PMU_REG_STATUS) {
        ++status_reads;
        if (fail_after_torn_status && status_reads == 2)
            return ESP_ERR_TIMEOUT;
        EXPECT_EQ(length, 64u);
        raw[4] = PMU_STATUS_BATTERY_VALID;
        put16(raw + 8, 3900);
        put16(raw + 12, 750);
        raw[16] = PMU_CHARGE_NOT_CHARGING;
    } else if (*reg == PMU_REG_RESPONSE) {
        EXPECT_EQ(length, 64u);
        std::memcpy(raw, request.data() + 1, 64);
        raw[2] = PMU_FRAME_KIND_RESP;
        raw[12] = 4;
        put16(raw + 18, 1500);
        raw[20] = no_vcom ? 0 : 1;
    } else {
        ADD_FAILURE() << "Unexpected register " << unsigned(*reg);
        return ESP_ERR_INVALID_ARG;
    }
    put16(raw + length - 2, read_pico_crc16(raw, length - 2));
    if (*reg == PMU_REG_STATUS && (corrupt_status || status_reads <= corrupt_status_reads))
        raw[4] ^= 1;
    return ESP_OK;
}
}

class ReadPicoBoardTest : public testing::Test
{
   protected:
    void SetUp() override
    {
        bus_count = device_count = calls = fail_at = commands = identity_reads = status_reads = 0;
        corrupt_status_reads = 0;
        fail_after_torn_status = false;
        simulate_pmu_reboot = no_vcom = corrupt_status = cleanup_failure = false;
        now = 100;
        ioe_output = 0x80;
        ioe_config = 0x64;
        ioe_input = 0x64;
        corrupt_ioe_readback = false;
    }
    void TearDown() override
    {
        cleanup_failure = false;
        EXPECT_EQ(read_pico_board_deinit(), ESP_OK);
        EXPECT_EQ(device_count, 0);
        EXPECT_EQ(bus_count, 0);
    }
};

TEST_F(ReadPicoBoardTest, SuccessfulProbeOnlySendsVcomGet)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    read_pico_board_snapshot_t state;
    ASSERT_EQ(read_pico_board_get_snapshot(&state), ESP_OK);
    EXPECT_EQ(commands, 1);
    EXPECT_TRUE(state.vcom_valid);
    EXPECT_EQ(state.vcom_mv, 1500);
    EXPECT_FALSE(state.sy_responding);
    EXPECT_EQ(state.status.battery_percent, 75);
    EXPECT_EQ(state.ioe_config, 0x64);
    EXPECT_FALSE(board_hal_is_usb_connected());
    EXPECT_EQ(board_hal_prepare_for_sleep(), ESP_ERR_NOT_SUPPORTED);
    EXPECT_EQ(board_hal_init(), ESP_ERR_INVALID_STATE);
}
TEST_F(ReadPicoBoardTest, ExpiredSnapshotReturnsUnknown)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    now += 5000001;
    read_pico_board_snapshot_t state;
    EXPECT_EQ(read_pico_board_get_snapshot(&state), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(state.status.battery_percent, -1);
    EXPECT_EQ(state.vcom_mv, -1);
}
TEST_F(ReadPicoBoardTest, AllRequiredIoFailuresReleaseOwnedResources)
{
    // SY probe is optional (call 8); all other operations are required.
    for (int stage : {1, 2, 3, 4, 5, 6, 7, 9, 10, 11}) {
        calls = identity_reads = 0;
        fail_at = stage;
        EXPECT_NE(board_hal_init(), ESP_OK) << stage;
        EXPECT_EQ(bus_count, 0) << stage;
        EXPECT_EQ(device_count, 0) << stage;
        read_pico_board_snapshot_t state;
        EXPECT_EQ(read_pico_board_get_snapshot(&state), ESP_ERR_INVALID_STATE);
    }
}
TEST_F(ReadPicoBoardTest, FailedRefreshInvalidatesPreviousSample)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    fail_at = calls + 1;
    EXPECT_EQ(read_pico_board_refresh(), ESP_ERR_TIMEOUT);
    read_pico_board_snapshot_t state;
    EXPECT_EQ(read_pico_board_get_snapshot(&state), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(state.vcom_valid);
}
TEST_F(ReadPicoBoardTest, CrcFailureDoesNotPublishSample)
{
    corrupt_status = true;
    EXPECT_EQ(board_hal_init(), ESP_ERR_INVALID_CRC);
    EXPECT_EQ(commands, 0);
    EXPECT_EQ(status_reads, 3);
}
TEST_F(ReadPicoBoardTest, TornStatusRecoversWithinThreeReadsBeforeOneGet)
{
    corrupt_status_reads = 2;
    ASSERT_EQ(board_hal_init(), ESP_OK);
    EXPECT_EQ(status_reads, 3);
    EXPECT_EQ(commands, 1);
    read_pico_board_snapshot_t state;
    ASSERT_EQ(read_pico_board_get_snapshot(&state), ESP_OK);
    EXPECT_TRUE(state.vcom_valid);
    EXPECT_EQ(state.vcom_mv, 1500);
}
TEST_F(ReadPicoBoardTest, PersistentStatusCrcInvalidatesPreviousValidSample)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    corrupt_status = true;
    status_reads = 0;
    EXPECT_EQ(read_pico_board_refresh(), ESP_ERR_INVALID_CRC);
    EXPECT_EQ(status_reads, 3);
    EXPECT_EQ(commands, 1);  // Only the previous successful refresh sent a GET.
    read_pico_board_snapshot_t state;
    EXPECT_EQ(read_pico_board_get_snapshot(&state), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(state.vcom_valid);
    EXPECT_EQ(state.vcom_mv, -1);
}
TEST_F(ReadPicoBoardTest, IoErrorAfterTornStatusReturnsImmediatelyWithoutGet)
{
    corrupt_status_reads = 1;
    fail_after_torn_status = true;
    EXPECT_EQ(board_hal_init(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(status_reads, 2);
    EXPECT_EQ(commands, 0);
    EXPECT_EQ(bus_count, 0);
    EXPECT_EQ(device_count, 0);
}
TEST_F(ReadPicoBoardTest, RebootDuringObservationRejectsMixedSession)
{
    simulate_pmu_reboot = true;
    EXPECT_EQ(board_hal_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(bus_count, 0);
}
TEST_F(ReadPicoBoardTest, MissingVcomLeavesDisplayGateClosed)
{
    no_vcom = true;
    ASSERT_EQ(board_hal_init(), ESP_OK);
    read_pico_board_snapshot_t state;
    ASSERT_EQ(read_pico_board_get_snapshot(&state), ESP_OK);
    EXPECT_FALSE(state.vcom_valid);
    EXPECT_EQ(state.vcom_mv, -1);
    EXPECT_EQ(commands, 1);  // Response polling doesn't resend or advance sequence.
}
TEST_F(ReadPicoBoardTest, CleanupFailureKeepsHandlesForRetry)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    cleanup_failure = true;
    EXPECT_EQ(read_pico_board_deinit(), ESP_ERR_TIMEOUT);
    EXPECT_EQ(device_count, 2);
    EXPECT_EQ(bus_count, 1);
    cleanup_failure = false;
    EXPECT_EQ(read_pico_board_deinit(), ESP_OK);
    EXPECT_EQ(device_count, 0);
}
TEST_F(ReadPicoBoardTest, RejectsAccessBeforeInitAndNullOutput)
{
    EXPECT_EQ(read_pico_board_refresh(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_board_get_snapshot(nullptr), ESP_ERR_INVALID_ARG);
}

TEST_F(ReadPicoBoardTest, DisplayLeasePreventsBusTeardownAndRequiresLock)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    i2c_master_bus_handle_t borrowed;
    EXPECT_EQ(read_pico_board_display_claim(&borrowed), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    ASSERT_EQ(read_pico_board_display_claim(&borrowed), ESP_OK);
    EXPECT_NE(borrowed, nullptr);
    EXPECT_EQ(read_pico_board_display_claim(&borrowed), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_board_deinit(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_board_display_release(), ESP_OK);
    read_pico_board_unlock();
    EXPECT_EQ(read_pico_board_deinit(), ESP_OK);
}

TEST_F(ReadPicoBoardTest, SharedRecursiveLockExcludesAnotherTask)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    esp_err_t other = ESP_OK;
    std::thread competitor([&]() { other = read_pico_board_lock(0); });
    competitor.join();
    EXPECT_EQ(other, ESP_ERR_TIMEOUT);
    read_pico_board_unlock();
    read_pico_board_unlock();
}

TEST_F(ReadPicoBoardTest, IoExpanderChangesOnlyDisplayOutputsAndPreservesTouch)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    i2c_master_bus_handle_t borrowed;
    ASSERT_EQ(read_pico_board_display_claim(&borrowed), ESP_OK);
    ASSERT_EQ(read_pico_board_ioe_configure_display(), ESP_OK);
    EXPECT_EQ(ioe_output, 0x81);
    EXPECT_EQ(ioe_config, 0x64);
    EXPECT_EQ(read_pico_board_ioe_update(0x80, 0), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_board_ioe_update(0x18, 0x08), ESP_OK);
    EXPECT_EQ(ioe_output, 0x89);
    EXPECT_EQ(read_pico_board_ioe_update(0x18, 0), ESP_OK);
    ASSERT_EQ(read_pico_board_display_release(), ESP_OK);
    read_pico_board_unlock();
}

TEST_F(ReadPicoBoardTest, RefusesLiveRailsAndDetectsReadbackFailure)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    i2c_master_bus_handle_t borrowed;
    ASSERT_EQ(read_pico_board_display_claim(&borrowed), ESP_OK);
    ioe_output = 0x88;
    EXPECT_EQ(read_pico_board_ioe_configure_display(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(ioe_output, 0x88);
    ioe_output = 0x80;
    corrupt_ioe_readback = true;
    EXPECT_EQ(read_pico_board_ioe_update(0x01, 0x01), ESP_ERR_INVALID_RESPONSE);
    corrupt_ioe_readback = false;
    ASSERT_EQ(read_pico_board_display_release(), ESP_OK);
    read_pico_board_unlock();
}

TEST_F(ReadPicoBoardTest, CardDetectIsFreshActiveLowAndReadOnly)
{
    ASSERT_EQ(board_hal_init(), ESP_OK);
    bool present = true;
    EXPECT_EQ(read_pico_board_sd_present(&present), ESP_OK);
    EXPECT_FALSE(present);
    ioe_input &= ~0x40;
    EXPECT_EQ(read_pico_board_sd_present(&present), ESP_OK);
    EXPECT_TRUE(present);
    EXPECT_EQ(ioe_config, 0x64);
    EXPECT_EQ(ioe_output, 0x80);
    EXPECT_EQ(commands, 1);  // Only startup VCOM GET; no power/calibration commands.
}

TEST_F(ReadPicoBoardTest, CardDetectErrorsNeverPublishPresence)
{
    bool present = true;
    EXPECT_EQ(read_pico_board_sd_present(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_board_sd_present(&present), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(present);
    ASSERT_EQ(board_hal_init(), ESP_OK);
    ioe_input = 0;
    for (int offset : {1, 2}) {
        present = true;
        fail_at = calls + offset;
        EXPECT_EQ(read_pico_board_sd_present(&present), ESP_ERR_TIMEOUT);
        EXPECT_FALSE(present);
    }
    fail_at = 0;
    ioe_config &= ~0x40;
    EXPECT_EQ(read_pico_board_sd_present(&present), ESP_ERR_INVALID_STATE);
    EXPECT_FALSE(present);
}

TEST_F(ReadPicoBoardTest, StorageLeaseRequiresLockAndPreventsHalDeinit)
{
    EXPECT_EQ(read_pico_board_storage_claim(), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(board_hal_init(), ESP_OK);
    EXPECT_EQ(read_pico_board_storage_claim(), ESP_ERR_INVALID_STATE);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    ASSERT_EQ(read_pico_board_storage_claim(), ESP_OK);
    EXPECT_EQ(read_pico_board_storage_claim(), ESP_ERR_INVALID_STATE);
    read_pico_board_unlock();
    EXPECT_EQ(read_pico_board_storage_release(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(read_pico_board_deinit(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(bus_count, 1);
    ASSERT_EQ(read_pico_board_lock(0), ESP_OK);
    EXPECT_EQ(read_pico_board_storage_release(), ESP_OK);
    EXPECT_EQ(read_pico_board_storage_release(), ESP_ERR_INVALID_STATE);
    read_pico_board_unlock();
    EXPECT_EQ(read_pico_board_deinit(), ESP_OK);
}
