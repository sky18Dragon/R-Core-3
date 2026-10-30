#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include "http_endpoint.h"
static bool fail_endpoint_allocation;
static unsigned endpoint_allocations;
extern "C" void *endpoint_test_malloc(size_t size) {
    if (fail_endpoint_allocation) return nullptr;
    void *value = malloc(size);
    if (value) ++endpoint_allocations;
    return value;
}
extern "C" void endpoint_test_free(void *value) {
    if (value) { EXPECT_GT(endpoint_allocations, 0u); --endpoint_allocations; }
    free(value);
}

TEST(HttpEndpoint, SchemePortQueryFragmentAndIpv6MatchSdkFields) {
    struct Case { const char *url; const char *host; unsigned port; bool tls; } cases[] = {
        {"https://example.invalid?q=1", "example.invalid", 443, true},
        {"HTTPS://EXAMPLE.invalid/#fragment", "EXAMPLE.invalid", 443, true},
        {"http://example.invalid:8080/a?q=1", "example.invalid", 8080, false},
        {"https://[2001:db8::1]:8443?key=value", "2001:db8::1", 8443, true},
        {"http://[::1]/", "::1", 80, false},
        {"https://host:65535/a", "host", 65535, true},
    };
    for (const auto &c : cases) {
        http_endpoint_t out{}; ASSERT_TRUE(http_endpoint_parse(c.url, &out)) << c.url;
        EXPECT_TRUE(http_endpoint_valid(c.url));
        EXPECT_STREQ(out.host, c.host); EXPECT_EQ(out.port, c.port); EXPECT_EQ(out.tls, c.tls);
    }
}
TEST(HttpEndpoint, InvalidAndCredentialUrlsNeverChangeOutput) {
    const char *cases[] = {nullptr, "", "/relative", "//host/path", "ftp://host", "https://",
        "https://user:secret@host/", "https://user@host/", "https://host:0/",
        "https://host:65536/", "https://host:443junk/", "https://[::1", "https://host:/",
        "https://host/a b", "https://host/\\path", "https://host/?q=\n",
        "https://host#fragment"}; // Fixed SDK requires a path/query before fragment.
    for (auto url : cases) {
        http_endpoint_t out; memset(&out, 0xa5, sizeof(out)); auto before = out;
        EXPECT_FALSE(http_endpoint_parse(url, &out)) << (url ? url : "NULL");
        EXPECT_FALSE(http_endpoint_valid(url));
        EXPECT_EQ(memcmp(&out, &before, sizeof(out)), 0);
    }
    EXPECT_FALSE(http_endpoint_parse("https://host", nullptr));
}
TEST(HttpEndpoint, HostAndInputBoundsRejectRatherThanTruncate) {
    http_endpoint_t out{};
    std::string host(255, 'a');
    ASSERT_TRUE(http_endpoint_parse(("https://" + host).c_str(), &out));
    EXPECT_EQ(strlen(out.host), 255u);
    EXPECT_FALSE(http_endpoint_parse(("https://" + host + "a").c_str(), &out));
    std::string url = "https://host/"; url.resize(2047, 'a');
    EXPECT_TRUE(http_endpoint_parse(url.c_str(), &out));
    url += 'a'; EXPECT_FALSE(http_endpoint_parse(url.c_str(), &out));
}
TEST(HttpEndpoint, RedirectRejectsEmbeddedAuthenticationBeforeSdkCanAdoptIt) {
    for (const char *bad : {"https://user:secret@host/x", "http://user@host/x",
         "//user:secret@host/x", "//user@host/x", "ftp://host/x", "data:text/plain,x",
         "https://host:0/x", "//host:65536/x", "//", "https://host/\r\nx"})
        EXPECT_FALSE(http_redirect_reference_valid(bad)) << bad;
    EXPECT_FALSE(http_redirect_reference_valid(nullptr));
    EXPECT_FALSE(http_redirect_reference_valid(""));
    std::string huge(2048, 'a'); EXPECT_FALSE(http_redirect_reference_valid(huge.c_str()));
}
TEST(HttpEndpoint, RedirectPreservesAbsoluteNetworkPathAndRelativeReferences) {
    for (const char *good : {"HTTPS://host:443/x", "http://[::1]:80/x", "//host/path",
         "//[::1]:8080/path", "/path", "../path", "image.jpg", "?q=1", "#part",
         "/path/https://literal", "/path?user=a@b", "./a:b", "/escaped%20path"})
        EXPECT_TRUE(http_redirect_reference_valid(good)) << good;
}
TEST(HttpEndpoint, NetworkReferenceAllocationFailureAndRejectionReleaseStorage) {
    fail_endpoint_allocation = true;
    EXPECT_FALSE(http_redirect_reference_valid("//host/path"));
    EXPECT_TRUE(http_redirect_reference_valid("/path"));
    fail_endpoint_allocation = false;
    EXPECT_TRUE(http_redirect_reference_valid("//host/path"));
    EXPECT_EQ(endpoint_allocations, 0u);
    EXPECT_FALSE(http_redirect_reference_valid("//user@host/path"));
    EXPECT_EQ(endpoint_allocations, 0u);
}
TEST(HttpEndpoint, HostHeaderUsesSchemeSpecificDefaultAndIpv6Brackets) {
    struct Case { const char *url; const char *expected; } cases[] = {
        {"http://host:80/x", "host"}, {"https://host:443/x", "host"},
        {"http://host:443/x", "host:443"}, {"https://host:80/x", "host:80"},
        {"https://[::1]/x", "[::1]"}, {"https://[::1]:80/x", "[::1]:80"},
        {"http://host:65535/x", "host:65535"}};
    for (const auto &c : cases) {
        char out[264]; ASSERT_TRUE(http_endpoint_host_header(c.url, out, sizeof(out)));
        EXPECT_STREQ(out, c.expected);
    }
}
TEST(HttpEndpoint, HostHeaderRejectsSmallOutputWithoutPublishingPrefix) {
    char out[16]; memset(out, '!', sizeof(out)); char before[16]; memcpy(before, out, sizeof(out));
    for (size_t cap = 0; cap < 9; ++cap) {
        EXPECT_FALSE(http_endpoint_host_header("http://host:443", out, cap));
        EXPECT_EQ(memcmp(out, before, sizeof(out)), 0);
    }
    EXPECT_FALSE(http_endpoint_host_header("https://user@host", out, sizeof(out)));
    EXPECT_EQ(memcmp(out, before, sizeof(out)), 0);
    EXPECT_TRUE(http_endpoint_host_header("http://host:443", out, 9));
    EXPECT_STREQ(out, "host:443"); EXPECT_EQ(out[9], '!');
    EXPECT_FALSE(http_endpoint_host_header("https://host", nullptr, 264));
}
