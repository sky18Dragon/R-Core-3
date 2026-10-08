#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include "wifi_link.h"
namespace { const uint8_t mac[6] = {1,2,3,4,5,6}; const uint8_t ssid[] = {'n','e','w'}; }
TEST(WifiLink, OnlyMatchingAssociationAndCurrentIpCanEstablishReady) {
    wifi_link_t s{}; ASSERT_TRUE(wifi_link_arm(&s, "new"));
    EXPECT_FALSE(wifi_link_ip(&s, ssid, 3, mac, true));
    ASSERT_TRUE(wifi_link_associated(&s, ssid, 3, mac));
    EXPECT_FALSE(wifi_link_ip(&s, ssid, 3, mac, false));
    EXPECT_FALSE(s.ready); EXPECT_TRUE(wifi_link_ip(&s, ssid, 3, mac, true)); EXPECT_TRUE(s.ready);
}
TEST(WifiLink, OldTargetAndDifferentBssidCannotConfirmNewAttempt) {
    wifi_link_t s{}; ASSERT_TRUE(wifi_link_arm(&s, "new"));
    EXPECT_FALSE(wifi_link_associated(&s, (const uint8_t *)"old", 3, mac));
    EXPECT_FALSE(wifi_link_ip(&s, ssid, 3, mac, true));
    ASSERT_TRUE(wifi_link_associated(&s, ssid, 3, mac));
    uint8_t different[6] = {6,5,4,3,2,1};
    EXPECT_FALSE(wifi_link_ip(&s, ssid, 3, different, true)); EXPECT_FALSE(s.ready);
}
TEST(WifiLink, CancelAndDisconnectInvalidateCachedReady) {
    wifi_link_t s{}; ASSERT_TRUE(wifi_link_arm(&s, "new"));
    ASSERT_TRUE(wifi_link_associated(&s, ssid, 3, mac)); ASSERT_TRUE(wifi_link_ip(&s, ssid, 3, mac, true));
    wifi_link_disconnected(&s); EXPECT_FALSE(s.ready); EXPECT_TRUE(s.active);
    EXPECT_FALSE(wifi_link_ip(&s, ssid, 3, mac, true));
    wifi_link_cancel(&s); EXPECT_FALSE(s.active);
    EXPECT_FALSE(wifi_link_associated(&s, ssid, 3, mac));
}
TEST(WifiLink, FullWireSsidAndNullMalformedBoundsAreChecked) {
    wifi_link_t s{}; std::string full(32, 'x'); ASSERT_TRUE(wifi_link_arm(&s, full.c_str()));
    EXPECT_TRUE(wifi_link_associated(&s, (const uint8_t *)full.data(), 32, mac));
    EXPECT_FALSE(wifi_link_associated(&s, nullptr, 32, mac));
    EXPECT_FALSE(wifi_link_associated(&s, (const uint8_t *)full.data(), 33, mac));
    uint8_t zero[6]{}; EXPECT_FALSE(wifi_link_associated(&s, (const uint8_t *)full.data(), 32, zero));
    EXPECT_FALSE(wifi_link_arm(&s, std::string(33, 'x').c_str()));
    EXPECT_FALSE(wifi_link_arm(&s, "")); EXPECT_FALSE(wifi_link_arm(&s, nullptr)); EXPECT_FALSE(s.ready);
}
TEST(WifiLink, ReassociationMustObtainVerifiedIpAgain) {
    wifi_link_t s{}; ASSERT_TRUE(wifi_link_arm(&s, "new"));
    ASSERT_TRUE(wifi_link_associated(&s, ssid, 3, mac)); ASSERT_TRUE(wifi_link_ip(&s, ssid, 3, mac, true));
    ASSERT_TRUE(wifi_link_associated(&s, ssid, 3, mac)); EXPECT_FALSE(s.ready);
    EXPECT_TRUE(wifi_link_ip(&s, ssid, 3, mac, true));
}
