#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
extern "C" {
#include "config_manager.h"
#include "esp_app_desc.h"
#include "mdns.h"
#include "mdns_service.h"
}
namespace {
std::string configured_name, recorded_host, recorded_instance, txt_name;
std::vector<std::string> calls;
esp_err_t copy_error, init_error, host_error, instance_error, http_error, photo_error;
bool mutate_at_init;
std::function<void()> init_hook;
class ReadPicoMdns : public testing::Test {
    void SetUp() override {
        configured_name = "synthetic"; calls.clear(); recorded_host.clear(); recorded_instance.clear(); txt_name.clear();
        copy_error = init_error = host_error = instance_error = http_error = photo_error = ESP_OK;
        mutate_at_init = false;
        init_hook = nullptr;
    }
};
}
extern "C" {
const char *esp_err_to_name(esp_err_t) { return "synthetic error"; }
esp_err_t config_manager_copy_device_name(char *out, size_t capacity) {
    calls.push_back("copy"); if (copy_error) return copy_error;
    if (configured_name.size() >= capacity) return ESP_ERR_INVALID_SIZE;
    memcpy(out, configured_name.c_str(), configured_name.size() + 1); return ESP_OK;
}
void sanitize_hostname(const char *name, char *out, size_t capacity) { snprintf(out, capacity, "%s", name); }
const esp_app_desc_t *esp_app_get_description(void) { static const esp_app_desc_t desc = {"synthetic"}; return &desc; }
esp_err_t mdns_init(void) {
    calls.push_back("init"); if (mutate_at_init) configured_name = "later";
    if (init_hook) { auto hook = std::move(init_hook); init_hook = nullptr; hook(); }
    return init_error;
}
void mdns_free(void) { calls.push_back("free"); }
esp_err_t mdns_hostname_set(const char *host) { calls.push_back("host"); recorded_host = host; return host_error; }
esp_err_t mdns_instance_name_set(const char *name) { calls.push_back("instance"); recorded_instance = name; return instance_error; }
esp_err_t mdns_service_add(const char *, const char *service, const char *, uint16_t port, mdns_txt_item_t *txt, size_t count) {
    EXPECT_EQ(port, 80);
    if (!strcmp(service, "_http")) { calls.push_back("http"); EXPECT_EQ(count, 0u); return http_error; }
    calls.push_back("photo"); EXPECT_EQ(count, 4u); txt_name = txt[0].value; return photo_error;
}
}
TEST_F(ReadPicoMdns, BusyInitSnapshotDoesNotCreateSdkService) {
    copy_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(mdns_service_init(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls, (std::vector<std::string>{"copy"}));
}
TEST_F(ReadPicoMdns, BusyUpdateSnapshotDoesNotFreeExistingService) {
    copy_error = ESP_ERR_INVALID_STATE;
    EXPECT_EQ(mdns_service_update_hostname(), ESP_ERR_INVALID_STATE);
    EXPECT_EQ(calls, (std::vector<std::string>{"copy"}));
}
TEST_F(ReadPicoMdns, InitUsesSingleOwnedNameDespiteLaterCacheChange) {
    mutate_at_init = true;
    ASSERT_EQ(mdns_service_init(), ESP_OK);
    EXPECT_EQ(recorded_host, "synthetic"); EXPECT_EQ(recorded_instance, "synthetic"); EXPECT_EQ(txt_name, "synthetic");
    EXPECT_EQ(configured_name, "later");
    EXPECT_EQ(calls, (std::vector<std::string>{"copy", "init", "host", "instance", "http", "photo"}));
}
TEST_F(ReadPicoMdns, UpdateCopiesBeforeFreeAndUsesSnapshotThroughout) {
    mutate_at_init = true;
    ASSERT_EQ(mdns_service_update_hostname(), ESP_OK);
    EXPECT_EQ(txt_name, "synthetic");
    EXPECT_EQ(calls, (std::vector<std::string>{"copy", "free", "init", "host", "instance", "http", "photo"}));
}
TEST_F(ReadPicoMdns, InitializationErrorDoesNotRegisterOrPretendRollback) {
    init_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(mdns_service_update_hostname(), ESP_ERR_NO_MEM);
    EXPECT_EQ(calls, (std::vector<std::string>{"copy", "free", "init"}));
}
TEST_F(ReadPicoMdns, HostAndInstanceErrorsStopRemainingRegistration) {
    host_error = ESP_FAIL; EXPECT_EQ(mdns_service_init(), ESP_FAIL);
    EXPECT_EQ(calls.back(), "host");
    calls.clear(); host_error = ESP_OK; instance_error = ESP_ERR_NO_MEM;
    EXPECT_EQ(mdns_service_init(), ESP_ERR_NO_MEM); EXPECT_EQ(calls.back(), "instance");
}
TEST_F(ReadPicoMdns, HttpErrorStopsOptionalDiscoveryRegistration) {
    http_error = ESP_FAIL;
    EXPECT_EQ(mdns_service_init(), ESP_FAIL); EXPECT_EQ(calls.back(), "http"); EXPECT_TRUE(txt_name.empty());
}
TEST_F(ReadPicoMdns, OptionalDiscoveryFailureRetainsUpstreamNonFatalContract) {
    photo_error = ESP_FAIL;
    EXPECT_EQ(mdns_service_init(), ESP_OK); EXPECT_EQ(calls.back(), "photo");
    // This result is not proof that optional discovery or asynchronous advertisement succeeded.
}
TEST_F(ReadPicoMdns, NamedUpdateUsesExplicitSnapshotWithoutConfigCopy) {
    copy_error = ESP_ERR_INVALID_STATE;
    ASSERT_EQ(mdns_service_update_hostname_named("explicit"), ESP_OK);
    EXPECT_EQ(recorded_host, "explicit"); EXPECT_EQ(txt_name, "explicit"); EXPECT_EQ(calls.front(), "free");
}
TEST_F(ReadPicoMdns, InvalidNamedUpdatesNeverFreeOrCallSdk) {
    for (const char *name : {"", "bad\nname", "bad\x7f"})
        EXPECT_EQ(mdns_service_update_hostname_named(name), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(mdns_service_update_hostname_named(nullptr), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(mdns_service_update_hostname_named(std::string(64, 'x').c_str()), ESP_ERR_INVALID_SIZE);
    EXPECT_TRUE(calls.empty());
}
TEST_F(ReadPicoMdns, ReentrantRegistrationDoesNotFreeActiveOperationAndErrorReleasesControl) {
    init_hook = [&] {
        EXPECT_EQ(mdns_service_update_hostname_named("nested"), ESP_ERR_INVALID_STATE);
    };
    init_error = ESP_FAIL;
    EXPECT_EQ(mdns_service_update_hostname_named("first"), ESP_FAIL);
    EXPECT_EQ(calls, (std::vector<std::string>{"free", "init"}));
    init_error = ESP_OK;
    EXPECT_EQ(mdns_service_update_hostname_named("retry"), ESP_OK); EXPECT_EQ(recorded_host, "retry");
}
