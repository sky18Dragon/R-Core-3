#include <gtest/gtest.h>
#include <cstring>
extern "C" {
#include "wifi_network_json.h"
}
namespace {
esp_err_t parse(const char *text, uint32_t *fields, wifi_network_t *patch) {
    cJSON *root = cJSON_Parse(text);
    esp_err_t result = wifi_network_json_patch(root, fields, patch);
    cJSON_Delete(root); return result;
}
}
TEST(WifiNetworkJson, FullStaticRequestProducesOnePatchWithoutIo) {
    uint32_t fields = 0; wifi_network_t patch{}, current{}, result{};
    ASSERT_EQ(parse(R"({"static_ip":"192.168.1.42","static_netmask":"255.255.255.0","static_gateway":"192.168.1.1","dns_server":"8.8.8.8","ip_mode":"static"})", &fields, &patch), ESP_OK);
    EXPECT_EQ(fields, WIFI_NETWORK_PATCH_ALL);
    ASSERT_TRUE(wifi_network_patch(&result, &current, fields, &patch));
    EXPECT_TRUE(result.static_address); EXPECT_STREQ(result.dns, "8.8.8.8");
}
TEST(WifiNetworkJson, AbsentFieldsDoNotSelectOrInventUpdates) {
    uint32_t fields = 99; wifi_network_t patch{};
    ASSERT_EQ(parse(R"({"timezone":"UTC0","unrelated":null})", &fields, &patch), ESP_OK);
    EXPECT_EQ(fields, 0u);
    ASSERT_EQ(parse(R"({"dns_server":""})", &fields, &patch), ESP_OK);
    EXPECT_EQ(fields, WIFI_NETWORK_PATCH_DNS); EXPECT_STREQ(patch.dns, "");
}
TEST(WifiNetworkJson, WrongKnownFieldTypesNeverPublish) {
    for (auto key : {"static_ip", "static_netmask", "static_gateway", "dns_server", "ip_mode"}) {
        for (auto value : {"null", "true", "3", "[]", "{}"}) {
            std::string json = std::string("{\"") + key + "\":" + value + "}";
            uint32_t fields = 42; wifi_network_t patch{}; strcpy(patch.dns, "unchanged");
            EXPECT_EQ(parse(json.c_str(), &fields, &patch), ESP_ERR_INVALID_ARG);
            EXPECT_EQ(fields, 42u); EXPECT_STREQ(patch.dns, "unchanged");
        }
    }
}
TEST(WifiNetworkJson, ExactModesOnlyAndCaseInsensitiveDuplicateKeysRejected) {
    for (auto text : {R"({"ip_mode":"STATIC"})", R"({"ip_mode":""})", R"({"ip_mode":"unknown"})",
                     R"({"static_ip":"","STATIC_IP":""})", R"({"ip_mode":"dhcp","IP_MODE":"static"})"}) {
        uint32_t fields = 42; wifi_network_t patch{};
        EXPECT_EQ(parse(text, &fields, &patch), ESP_ERR_INVALID_ARG); EXPECT_EQ(fields, 42u);
    }
    uint32_t fields; wifi_network_t patch{};
    EXPECT_EQ(parse(R"({"DNS_SERVER":"1.1.1.1","ip_mode":"dhcp"})", &fields, &patch), ESP_OK);
    EXPECT_EQ(fields, WIFI_NETWORK_PATCH_DNS | WIFI_NETWORK_PATCH_MODE);
}
TEST(WifiNetworkJson, CapacityRejectsTruncationAndLeavesOutputs) {
    uint32_t fields = 42; wifi_network_t patch{};
    EXPECT_EQ(parse(R"({"dns_server":"1111111111111111"})", &fields, &patch), ESP_ERR_INVALID_SIZE);
    EXPECT_EQ(fields, 42u);
    EXPECT_EQ(parse(R"({"dns_server":"255.255.255.255"})", &fields, &patch), ESP_OK);
    EXPECT_STREQ(patch.dns, "255.255.255.255");
}
TEST(WifiNetworkJson, NonObjectAndNullOutputsAreRejected) {
    uint32_t fields = 42; wifi_network_t patch{};
    for (auto text : {"null", "[]", "1", "\"x\""}) {
        EXPECT_EQ(parse(text, &fields, &patch), ESP_ERR_INVALID_ARG); EXPECT_EQ(fields, 42u);
    }
    EXPECT_EQ(parse("{}", nullptr, &patch), ESP_ERR_INVALID_ARG);
    EXPECT_EQ(parse("{}", &fields, nullptr), ESP_ERR_INVALID_ARG);
}
TEST(WifiNetworkJson, EffectiveStaticSetRejectsMissingOrClearedAddresses) {
    uint32_t fields; wifi_network_t patch{}, current{}, result{};
    ASSERT_EQ(parse(R"({"ip_mode":"static"})", &fields, &patch), ESP_OK);
    EXPECT_FALSE(wifi_network_patch(&result, &current, fields, &patch));
    ASSERT_TRUE(wifi_network_copy(&current, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", ""));
    ASSERT_EQ(parse(R"({"static_ip":""})", &fields, &patch), ESP_OK);
    EXPECT_FALSE(wifi_network_patch(&result, &current, fields, &patch));
    ASSERT_EQ(parse(R"({"static_ip":"","ip_mode":"dhcp"})", &fields, &patch), ESP_OK);
    EXPECT_TRUE(wifi_network_patch(&result, &current, fields, &patch));
    EXPECT_FALSE(result.static_address); EXPECT_STREQ(result.ip, "");
}
