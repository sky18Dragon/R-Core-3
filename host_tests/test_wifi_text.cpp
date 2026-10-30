#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include "wifi_text.h"
namespace {
bool parse(const std::string &s, wifi_text_t *out) {
    return wifi_text_parse(reinterpret_cast<const uint8_t *>(s.data()), s.size(), out);
}
bool form(const std::string &s) {
    return wifi_text_form_valid(reinterpret_cast<const uint8_t *>(s.data()), s.size());
}
}
TEST(WifiText, LfCrLfCommentsSeparatorsAndOptionalNamePreserveExactBytes) {
    for (const std::string &s : {std::string("# setup\n\nMy Network\n\npassword\nFrame\n# end\n"),
                               std::string("My Network\r\npassword\r\nFrame")}) {
        wifi_text_t out{}; ASSERT_TRUE(parse(s, &out));
        EXPECT_STREQ(out.ssid, "My Network"); EXPECT_STREQ(out.password, "password");
        EXPECT_TRUE(out.has_device_name); EXPECT_STREQ(out.device_name, "Frame");
    }
}
TEST(WifiText, TwoFieldsWithoutFinalNewlineHaveNoName) {
    wifi_text_t out{}; ASSERT_TRUE(parse("ssid\npassword", &out));
    EXPECT_FALSE(out.has_device_name); EXPECT_STREQ(out.device_name, "");
}
TEST(WifiText, ExplicitOpenMarkerKeepsLegacyBlankSeparatorMeaning) {
    wifi_text_t out{}; ASSERT_TRUE(parse("ssid\n\n\"\"\nFrame", &out));
    EXPECT_STREQ(out.password, ""); EXPECT_STREQ(out.device_name, "Frame");
    ASSERT_TRUE(parse("ssid\n\npassword", &out)); EXPECT_STREQ(out.password, "password");
}
TEST(WifiText, FullWireSsidHexPskAndNameFitWithoutTruncation) {
    std::string ssid(32, 's'), psk(64, 'A'), name(63, 'n'); wifi_text_t out{};
    ASSERT_TRUE(parse(ssid + "\n" + psk + "\n" + name, &out));
    EXPECT_EQ(std::string(out.ssid), ssid); EXPECT_EQ(std::string(out.password), psk);
    EXPECT_EQ(std::string(out.device_name), name);
}
TEST(WifiText, OverlongFieldAndFourthFieldNeverChangeOutput) {
    wifi_text_t out; memset(&out, 0x5a, sizeof(out)); wifi_text_t previous = out;
    for (const std::string &s : {std::string(33, 's') + "\npassword", "ssid\n" + std::string(65, 'a'),
            "ssid\npassword\n" + std::string(64, 'n'), std::string("ssid\npassword\nFrame\nextra")}) {
        EXPECT_FALSE(parse(s, &out)); EXPECT_EQ(memcmp(&out, &previous, sizeof(out)), 0);
    }
}
TEST(WifiText, WholeFileIncludingCommentsRejectsNulControlAndTruncatedCrLf) {
    wifi_text_t out{};
    for (const std::string &s : {std::string("ssid\npassword\n#bad\0data", 23),
            std::string("ssid\npassword\r"), std::string("ssid\rpassword"),
            std::string("ssid\npass\tword"), std::string("ssid\npassword\n#\x7f")})
        EXPECT_FALSE(parse(s, &out));
}
TEST(WifiText, EmptyMissingOrOverBudgetAndNullInputsAreRejected) {
    wifi_text_t out{};
    for (const std::string &s : {std::string(""), std::string("# comment\n\n"), std::string("ssid\n"),
                                std::string(1025, '#')}) EXPECT_FALSE(parse(s, &out));
    EXPECT_FALSE(wifi_text_parse(nullptr, 5, &out)); EXPECT_FALSE(parse("ssid\npassword", nullptr));
}
TEST(WifiText, SpacesAndMultibyteSsidUseByteLimitsNotCharacterCounts) {
    wifi_text_t out{}; std::string ssid = " 网络 ";
    ASSERT_TRUE(parse(ssid + "\npass word", &out)); EXPECT_EQ(std::string(out.ssid), ssid);
    EXPECT_STREQ(out.password, "pass word");
}
TEST(WifiText, CredentialsSupportOpenPassphraseAndHexButNeverMalformedPsk) {
    EXPECT_TRUE(wifi_text_credentials_valid("network", ""));
    EXPECT_TRUE(wifi_text_credentials_valid("#network", "#password"));
    EXPECT_TRUE(wifi_text_credentials_valid(std::string(32, 's').c_str(), std::string(64, 'f').c_str()));
    EXPECT_FALSE(wifi_text_credentials_valid("", "password"));
    EXPECT_FALSE(wifi_text_credentials_valid(nullptr, "password"));
    EXPECT_FALSE(wifi_text_credentials_valid("ssid", nullptr));
    EXPECT_FALSE(wifi_text_credentials_valid("ssid", "short"));
    EXPECT_FALSE(wifi_text_credentials_valid("ssid", std::string(64, 'g').c_str()));
    EXPECT_FALSE(wifi_text_credentials_valid("ssid", "password\n"));
    EXPECT_FALSE(wifi_text_credentials_valid("ssid", "密码password"));
}
TEST(WifiForm, CompleteOrderIndependentBodyAndPercentEncodedBoundaries) {
    EXPECT_TRUE(form("password=pass+word&ssid=My%20Network&deviceName=Frame&ipMode=dhcp"));
    EXPECT_TRUE(form("ssid=" + std::string(32, 's') + "&password=" + std::string(64, 'f')));
    std::string encoded;
    for (int i = 0; i < 32; ++i) encoded += "%61";
    EXPECT_TRUE(form("ssid=" + encoded + "&password="));
    EXPECT_TRUE(form("ssid=network&password=&futureOption=value"));
}
TEST(WifiForm, DuplicateBadEscapesBinaryControlAndIncompleteFieldsFail) {
    for (const std::string &s : {std::string("ssid=x&ssid=y"), std::string("ssid=x&password=%00abcdefgh"),
        std::string("ssid=%GG"), std::string("ssid=%"), std::string("ssid=%1"), std::string("ssid=x&"),
        std::string("ssid=x&&password=p"), std::string("ssid=x&password"), std::string("ss%69d=x"),
        std::string("password=password"), std::string("ssid=x&deviceName=a%0Ab"),
        std::string("ssid=x&bad key=value"), std::string("ssid=\0x", 7)}) EXPECT_FALSE(form(s)) << s;
}
TEST(WifiForm, EveryKnownFieldLimitRejectsPrefixesAndWholeBodyIsBounded) {
    for (const auto &field : {std::pair<const char *, int>{"ssid", 32}, {"password", 64},
            {"deviceName", 63}, {"ipMode", 7}, {"staticIp", 15}, {"staticNetmask", 15},
            {"staticGateway", 15}, {"dnsServer", 15}}) {
        std::string s = std::string(field.first) + "=" + std::string(field.second + 1, 'a');
        if (strcmp(field.first, "ssid")) s += "&ssid=x";
        EXPECT_FALSE(form(s));
    }
    EXPECT_FALSE(form("ssid=" + std::string(512, 's')));
    EXPECT_FALSE(wifi_text_form_valid(nullptr, 3));
}
