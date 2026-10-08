#include <gtest/gtest.h>
#include <cstring>
#include <string>
extern "C" {
#include "wifi_network.h"
}
TEST(WifiNetwork, StaticAndDhcpDnsAreCheckedWithoutIo) {
    wifi_network_t n{};
    ASSERT_TRUE(wifi_network_copy(&n, true, "192.168.1.42", "255.255.255.0", "192.168.1.1", "8.8.8.8"));
    EXPECT_TRUE(n.static_address); EXPECT_STREQ(n.ip, "192.168.1.42");
    EXPECT_TRUE(wifi_network_valid(&n));
    EXPECT_TRUE(wifi_network_copy(&n, false, "retained", "retained", "retained", ""));
    EXPECT_FALSE(n.static_address); EXPECT_STREQ(n.ip, "retained");
}
TEST(WifiNetwork, InvalidOctetsAndMalformedFormsKeepOutput) {
    wifi_network_t n{}, old{}; n.dns[0] = 'x'; old = n;
    for (auto value : {"", "1.2.3", "1.2.3.4.5", ".1.2.3", "1..2.3", "256.1.2.3",
                       "1.2.3.999", "1.2.3.-1", "0x1.2.3.4", " 1.2.3.4", "1.2.3.4 ",
                       "1.2.3.4\n", "0000.1.2.3", "010.2.3.4", "1.02.3.4", "1.2.03.4", "1.2.3.00"}) {
        EXPECT_FALSE(wifi_network_copy(&n, true, value, "255.255.255.0", "1.2.3.1", "")) << value;
        EXPECT_EQ(memcmp(&n, &old, sizeof(n)), 0);
    }
}
TEST(WifiNetwork, EachRequiredStaticFieldAndDnsIsValidated) {
    wifi_network_t n{};
    EXPECT_FALSE(wifi_network_copy(&n, true, "1.2.3.4", "bad", "1.2.3.1", ""));
    EXPECT_FALSE(wifi_network_copy(&n, true, "1.2.3.4", "255.255.255.0", "bad", ""));
    EXPECT_FALSE(wifi_network_copy(&n, false, "", "", "", "bad"));
    EXPECT_TRUE(wifi_network_copy(&n, true, "0.0.0.0", "255.255.255.255", "0.0.0.0", "255.255.255.255"));
    // Syntax check only: no invented routing/subnet or reachability guarantees.
}
TEST(WifiNetwork, FullCapacityAndNullInputsNeverTruncateOrPublish) {
    wifi_network_t n{}, old{}; n.dns[0] = 'x'; old = n;
    auto too_long = std::string(16, '1');
    EXPECT_FALSE(wifi_network_copy(&n, false, too_long.c_str(), "", "", ""));
    EXPECT_FALSE(wifi_network_copy(&n, false, "", too_long.c_str(), "", ""));
    EXPECT_FALSE(wifi_network_copy(&n, false, "", "", too_long.c_str(), ""));
    EXPECT_FALSE(wifi_network_copy(&n, false, "", "", "", too_long.c_str()));
    EXPECT_FALSE(wifi_network_copy(&n, false, nullptr, "", "", ""));
    EXPECT_FALSE(wifi_network_copy(nullptr, false, "", "", "", ""));
    EXPECT_FALSE(wifi_network_valid(nullptr)); EXPECT_EQ(memcmp(&n, &old, sizeof(n)), 0);
}
TEST(WifiNetwork, DirectNonterminatedStructuresAreRejectedInEveryField) {
    for (int i = 0; i < 4; ++i) {
        wifi_network_t n{}; char *fields[] = {n.ip, n.netmask, n.gateway, n.dns};
        memset(fields[i], '1', 16); EXPECT_FALSE(wifi_network_valid(&n));
    }
}
TEST(WifiNetwork, EmptyStaticFailsButDhcpCanKeepUnusedValues) {
    wifi_network_t n{};
    EXPECT_FALSE(wifi_network_copy(&n, true, "", "", "", ""));
    EXPECT_TRUE(wifi_network_copy(&n, false, "", "", "", ""));
    EXPECT_TRUE(wifi_network_copy(&n, false, "old", "old", "old", "1.1.1.1"));
    EXPECT_TRUE(wifi_network_copy(&n, true, "255.255.255.255", "255.255.255.255", "255.255.255.255", ""));
}
TEST(WifiNetwork, PatchCopiesOnlySelectedFieldsAndSupportsAliasedOutput) {
    wifi_network_t current{}, patch{};
    ASSERT_TRUE(wifi_network_copy(&current, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", ""));
    strcpy(patch.dns, "8.8.8.8"); memset(patch.ip, 'x', 16);
    ASSERT_TRUE(wifi_network_patch(&current, &current, WIFI_NETWORK_PATCH_DNS, &patch));
    EXPECT_STREQ(current.ip, "1.2.3.4"); EXPECT_STREQ(current.dns, "8.8.8.8");
    EXPECT_TRUE(current.static_address);
}
TEST(WifiNetwork, PatchBadMaskAndNullPointersLeaveOutput) {
    wifi_network_t current{}, patch{}, out{}; strcpy(out.ip, "unchanged");
    EXPECT_FALSE(wifi_network_patch(&out, &current, 32, &patch));
    EXPECT_FALSE(wifi_network_patch(&out, nullptr, 0, &patch));
    EXPECT_FALSE(wifi_network_patch(&out, &current, 0, nullptr));
    EXPECT_FALSE(wifi_network_patch(nullptr, &current, 0, &patch));
    EXPECT_STREQ(out.ip, "unchanged");
}
TEST(WifiNetwork, PatchEveryMalformedSelectedFieldLeavesWholeOutput) {
    for (int i = 0; i < 4; ++i) {
        wifi_network_t current{}, patch{}, out{}; strcpy(out.ip, "unchanged");
        char *values[] = {patch.ip, patch.netmask, patch.gateway, patch.dns};
        strcpy(values[i], "bad");
        EXPECT_FALSE(wifi_network_patch(&out, &current, 1u << i, &patch));
        memset(values[i], '1', 16);
        EXPECT_FALSE(wifi_network_patch(&out, &current, 1u << i, &patch));
        EXPECT_STREQ(out.ip, "unchanged");
    }
}
TEST(WifiNetwork, FullPatchCanRepairInvalidRetainedSnapshotWithoutReadingStrings) {
    wifi_network_t current{}, patch{}, out{};
    memset(current.ip, 'x', 16); strcpy(current.dns, "bad");
    ASSERT_TRUE(wifi_network_copy(&patch, true, "1.2.3.4", "255.255.255.0", "1.2.3.1", ""));
    EXPECT_TRUE(wifi_network_patch(&out, &current, WIFI_NETWORK_PATCH_ALL, &patch));
    EXPECT_TRUE(wifi_network_valid(&out));
    EXPECT_FALSE(wifi_network_patch(&out, &current, WIFI_NETWORK_PATCH_MODE, &patch));
}
TEST(WifiNetwork, DhcpRetainsUnusedTextButNewSelectedValuesMustBeIpv4) {
    wifi_network_t current{}, patch{}, out{};
    ASSERT_TRUE(wifi_network_copy(&current, false, "old", "old", "old", ""));
    strcpy(patch.dns, "1.1.1.1");
    EXPECT_TRUE(wifi_network_patch(&out, &current, WIFI_NETWORK_PATCH_DNS, &patch));
    EXPECT_STREQ(out.ip, "old");
    strcpy(patch.ip, "new");
    EXPECT_FALSE(wifi_network_patch(&out, &current, WIFI_NETWORK_PATCH_IP, &patch));
    patch.static_address = true;
    EXPECT_FALSE(wifi_network_patch(&out, &current, WIFI_NETWORK_PATCH_MODE, &patch));
}
