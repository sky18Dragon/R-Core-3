#include <gtest/gtest.h>

#include <array>
#include <vector>

extern "C" {
#include "read_pico_sy_power.h"
}

namespace
{
std::array<uint8_t, 9> registers;
std::vector<std::array<uint8_t, 2>> writes;
std::vector<bool> en_calls, vcom_calls;
bool en_state, vcom_state, pointer_pending;
int pg_value, pg_calls, fail_write_reg, fail_read_reg, devices;
uint8_t selected;
esp_err_t add_error, remove_error, en_on_error, en_off_error, vcom_on_error, vcom_off_error;
int bus_token, device_token;
i2c_master_bus_handle_t bus = reinterpret_cast<i2c_master_bus_handle_t>(&bus_token);
i2c_master_dev_handle_t device = reinterpret_cast<i2c_master_dev_handle_t>(&device_token);

esp_err_t en(bool on)
{
    en_calls.push_back(on);
    esp_err_t error = on ? en_on_error : en_off_error;
    if (error == ESP_OK)
        en_state = on;
    return error;
}
esp_err_t vcom(bool on)
{
    vcom_calls.push_back(on);
    esp_err_t error = on ? vcom_on_error : vcom_off_error;
    if (error == ESP_OK)
        vcom_state = on;
    return error;
}
int pg()
{
    ++pg_calls;
    return pg_value;
}

class SyPower : public testing::Test
{
   protected:
    read_pico_sy_handle_t handle = nullptr;
    void SetUp() override
    {
        registers.fill(0);
        registers[2] = 0x94;
        registers[7] = 1;
        registers[8] = 24;
        writes.clear();
        en_calls.clear();
        vcom_calls.clear();
        en_state = vcom_state = pointer_pending = false;
        pg_value = 1;
        pg_calls = devices = 0;
        fail_write_reg = fail_read_reg = -1;
        add_error = remove_error = en_on_error = en_off_error = ESP_OK;
        vcom_on_error = vcom_off_error = ESP_OK;
    }
    void create(int mv = 1250)
    {
        ASSERT_EQ(read_pico_sy_init(bus, mv, en, vcom, pg, &handle), ESP_OK);
    }
    void TearDown() override
    {
        fail_write_reg = fail_read_reg = -1;
        en_off_error = vcom_off_error = remove_error = ESP_OK;
        EXPECT_EQ(read_pico_sy_deinit(&handle), ESP_OK);
        EXPECT_EQ(handle, nullptr);
        EXPECT_EQ(devices, 0);
    }
};
}  // namespace

extern "C" {
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t supplied,
                                    const i2c_device_config_t *cfg, i2c_master_dev_handle_t *out)
{
    EXPECT_EQ(supplied, bus);
    EXPECT_EQ(cfg->device_address, 0x62);
    EXPECT_EQ(cfg->scl_speed_hz, 400000U);
    if (add_error != ESP_OK)
        return add_error;
    ++devices;
    *out = device;
    return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t supplied)
{
    EXPECT_EQ(supplied, device);
    if (remove_error != ESP_OK)
        return remove_error;
    --devices;
    return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t supplied, const uint8_t *data, size_t size,
                              int timeout)
{
    EXPECT_EQ(supplied, device);
    EXPECT_EQ(timeout, 100);
    EXPECT_TRUE(en_state);
    if (size == 1) {
        selected = data[0];
        pointer_pending = true;
        return ESP_OK;
    }
    EXPECT_EQ(size, 2U);
    EXPECT_FALSE(pointer_pending);
    if (size != 2 || data[0] >= registers.size())
        return ESP_FAIL;
    writes.push_back({data[0], data[1]});
    if (data[0] == fail_write_reg)
        return ESP_ERR_TIMEOUT;
    registers[data[0]] = data[1];
    return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t supplied, uint8_t *data, size_t size,
                             int timeout)
{
    EXPECT_EQ(supplied, device);
    EXPECT_EQ(size, 1U);
    EXPECT_EQ(timeout, 100);
    EXPECT_TRUE(pointer_pending);
    EXPECT_TRUE(en_state);
    pointer_pending = false;
    if (selected == fail_read_reg)
        return ESP_ERR_INVALID_RESPONSE;
    if (selected >= registers.size())
        return ESP_FAIL;
    *data = registers[selected];
    return ESP_OK;
}
esp_err_t gpio_set_level(gpio_num_t, uint32_t)
{
    ADD_FAILURE();
    return ESP_FAIL;
}
int gpio_get_level(gpio_num_t)
{
    ADD_FAILURE();
    return -1;
}
esp_err_t gpio_config(const gpio_config_t *)
{
    ADD_FAILURE();
    return ESP_FAIL;
}
void esp_rom_delay_us(uint32_t) {}
int64_t esp_timer_get_time(void)
{
    static int64_t clock = 0;
    return clock += 10000;
}
const char *esp_err_to_name(esp_err_t)
{
    return "injected";
}
}

TEST_F(SyPower, InvalidFactoryVcomCannotAllocateOrActivate)
{
    for (int mv : {0, 499, 501, 2501, 5000})
        EXPECT_EQ(read_pico_sy_init(bus, mv, en, vcom, pg, &handle), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(read_pico_sy_init(nullptr, 1250, en, vcom, pg, &handle), ESP_ERR_INVALID_ARG);
    EXPECT_TRUE(en_calls.empty());
    EXPECT_TRUE(writes.empty());
    EXPECT_EQ(devices, 0);
}
TEST_F(SyPower, InitIsPassiveAndUsesOnlySyAddress)
{
    create();
    EXPECT_EQ(read_pico_sy_get_vcom(handle), 1250);
    EXPECT_TRUE(en_calls.empty());
    EXPECT_TRUE(vcom_calls.empty());
    EXPECT_TRUE(writes.empty());
}
TEST_F(SyPower, DeviceAddFailureDoesNotLeak)
{
    add_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(read_pico_sy_init(bus, 1250, en, vcom, pg, &handle), ESP_ERR_NO_MEM);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(devices, 0);
    EXPECT_TRUE(en_calls.empty());
}
TEST_F(SyPower, StandardStartupPreservesReservedBitsAndWriteWhitelist)
{
    create(2500);
    ASSERT_EQ(read_pico_sy_power_on(handle), ESP_OK);
    const std::vector<std::array<uint8_t, 2>> expected = {
        {1, 250}, {2, 0x14}, {3, 0x66}, {6, 0xaa}, {0, 0xc0}};
    EXPECT_EQ(writes, expected);
    EXPECT_TRUE(vcom_state);
    EXPECT_EQ(pg_calls, 1);
    const auto count = writes.size();
    EXPECT_EQ(read_pico_sy_power_on(handle), ESP_OK);
    EXPECT_EQ(writes.size(), count);
}
TEST_F(SyPower, DigitalTemperatureReadDoesNotStartRailsOrWriteRegisters)
{
    create();
    bool woke = false;
    ASSERT_EQ(read_pico_sy_i2c_begin(handle, &woke), ESP_OK);
    EXPECT_TRUE(woke);
    sy7636a_status_t state;
    ASSERT_EQ(read_pico_sy_read(handle, &state), ESP_OK);
    EXPECT_EQ(state.temperature_c, 24);
    EXPECT_FALSE(state.on);
    EXPECT_TRUE(state.pg);
    EXPECT_EQ(state.fault, SY7636A_FAULT_NONE);
    EXPECT_TRUE(writes.empty());
    EXPECT_TRUE(vcom_calls.empty());
    EXPECT_EQ(read_pico_sy_i2c_end(handle, woke), ESP_OK);
    EXPECT_FALSE(en_state);
}
TEST_F(SyPower, ReadUsesSignedTemperatureAndRealVendorFaultDecoder)
{
    create();
    bool woke;
    ASSERT_EQ(read_pico_sy_i2c_begin(handle, &woke), ESP_OK);
    registers[8] = 0xfb;
    registers[7] = (15 << 1) | 1;
    sy7636a_status_t state;
    ASSERT_EQ(read_pico_sy_read(handle, &state), ESP_OK);
    EXPECT_EQ(state.temperature_c, -5);
    EXPECT_EQ(state.fault, SY7636A_FAULT_OTP);
    registers[7] = 14 << 1;
    ASSERT_EQ(read_pico_sy_read(handle, &state), ESP_OK);
    EXPECT_EQ(state.fault, SY7636A_FAULT_UNKNOWN);
    EXPECT_FALSE(state.pg);
}
TEST_F(SyPower, ReadFailureReturnsErrorWithoutFabricatedTemperature)
{
    create();
    bool woke;
    ASSERT_EQ(read_pico_sy_i2c_begin(handle, &woke), ESP_OK);
    fail_read_reg = 8;
    sy7636a_status_t state;
    EXPECT_EQ(read_pico_sy_read(handle, &state), ESP_ERR_INVALID_RESPONSE);
}
TEST_F(SyPower, PgoodTimeoutIsBoundedAndCannotEnableVcom)
{
    create();
    pg_value = 0;
    EXPECT_EQ(read_pico_sy_power_on(handle), ESP_ERR_TIMEOUT);
    EXPECT_EQ(pg_calls, 30);
    EXPECT_FALSE(vcom_state);
    EXPECT_EQ(read_pico_sy_power_off(handle), ESP_OK);
    EXPECT_FALSE(en_state);
}
TEST_F(SyPower, PgoodIoErrorIsNotTruthySuccess)
{
    create();
    pg_value = -1;
    EXPECT_EQ(read_pico_sy_power_on(handle), ESP_FAIL);
    EXPECT_EQ(pg_calls, 1);
    EXPECT_FALSE(vcom_state);
}
TEST_F(SyPower, EachPartialStartupWriteFailureCanBeStopped)
{
    create();
    for (int reg : {1, 2, 3, 6, 0}) {
        fail_write_reg = reg;
        EXPECT_EQ(read_pico_sy_power_on(handle), ESP_ERR_TIMEOUT);
        EXPECT_FALSE(vcom_state);
        fail_write_reg = -1;
        EXPECT_EQ(read_pico_sy_power_off(handle), ESP_OK);
        EXPECT_FALSE(en_state);
    }
}
TEST_F(SyPower, EnableFailureDoesNotAttemptI2cOrVcom)
{
    create();
    en_on_error = ESP_ERR_INVALID_RESPONSE;
    EXPECT_EQ(read_pico_sy_power_on(handle), ESP_ERR_INVALID_RESPONSE);
    EXPECT_TRUE(writes.empty());
    EXPECT_TRUE(vcom_calls.empty());
}
TEST_F(SyPower, VcomEnableFailureIsReturnedAndCleanupStillWorks)
{
    create();
    vcom_on_error = ESP_ERR_INVALID_RESPONSE;
    EXPECT_EQ(read_pico_sy_power_on(handle), ESP_ERR_INVALID_RESPONSE);
    EXPECT_FALSE(vcom_state);
    EXPECT_EQ(read_pico_sy_power_off(handle), ESP_OK);
}
TEST_F(SyPower, ShutdownAttemptsAllStepsAndPreservesFirstError)
{
    create();
    ASSERT_EQ(read_pico_sy_power_on(handle), ESP_OK);
    vcom_off_error = ESP_ERR_INVALID_RESPONSE;
    fail_write_reg = 0;
    EXPECT_EQ(read_pico_sy_power_off(handle), ESP_ERR_INVALID_RESPONSE);
    EXPECT_EQ(en_calls.back(), false);
    EXPECT_FALSE(en_state);
    EXPECT_EQ(writes.back()[0], 0);
    EXPECT_EQ(writes.back()[1], 0);
}
TEST_F(SyPower, DigitalEndFailureRetainsStateForShutdownRetry)
{
    create();
    bool woke;
    ASSERT_EQ(read_pico_sy_i2c_begin(handle, &woke), ESP_OK);
    en_off_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sy_i2c_end(handle, woke), ESP_ERR_TIMEOUT);
    EXPECT_TRUE(en_state);
    en_off_error = ESP_OK;
    EXPECT_EQ(read_pico_sy_power_off(handle), ESP_OK);
    EXPECT_FALSE(en_state);
}
TEST_F(SyPower, DeinitRetainsDeviceOnShutdownAndRemovalFailure)
{
    create();
    ASSERT_EQ(read_pico_sy_power_on(handle), ESP_OK);
    auto original = handle;
    en_off_error = ESP_ERR_TIMEOUT;
    EXPECT_EQ(read_pico_sy_deinit(&handle), ESP_ERR_TIMEOUT);
    EXPECT_EQ(handle, original);
    EXPECT_EQ(devices, 1);
    en_off_error = ESP_OK;
    remove_error = ESP_FAIL;
    EXPECT_EQ(read_pico_sy_deinit(&handle), ESP_FAIL);
    EXPECT_EQ(handle, original);
    EXPECT_EQ(devices, 1);
    remove_error = ESP_OK;
    EXPECT_EQ(read_pico_sy_deinit(&handle), ESP_OK);
    EXPECT_EQ(handle, nullptr);
}
