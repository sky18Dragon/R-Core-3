#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "upload_stream.h"

namespace
{
struct Sink {
    std::vector<std::string> fields, names, payload;
    int ends = 0;
    bool fail_begin = false, fail_write = false, fail_end = false;
    static bool begin(void *p, const char *field, const char *name)
    {
        auto &s = *static_cast<Sink *>(p);
        s.fields.emplace_back(field);
        s.names.emplace_back(name);
        s.payload.emplace_back();
        return !s.fail_begin;
    }
    static bool write(void *p, const uint8_t *bytes, size_t n)
    {
        auto &s = *static_cast<Sink *>(p);
        if (s.fail_write)
            return false;
        s.payload.back().append(reinterpret_cast<const char *>(bytes), n);
        return true;
    }
    static bool end(void *p)
    {
        auto &s = *static_cast<Sink *>(p);
        ++s.ends;
        return !s.fail_end;
    }
    upload_stream_sink_t io()
    {
        return {this, begin, write, end};
    }
};
std::string part(const std::string &field, const std::string &name, const std::string &data)
{
    return "--abc\r\nContent-Disposition: form-data; name=\"" + field + "\"; filename=\"" + name +
           "\"\r\nContent-Type: image/jpeg\r\n\r\n" + data + "\r\n";
}
bool run(Sink &s, const std::string &input, size_t chunk = 4096, size_t limit = 1024 * 1024)
{
    upload_stream_t parser;
    auto io = s.io();
    if (!upload_stream_init(&parser, "abc", limit, &io))
        return false;
    for (size_t i = 0; i < input.size(); i += chunk)
        if (!upload_stream_feed(&parser, input.data() + i, std::min(chunk, input.size() - i)))
            return false;
    return upload_stream_finish(&parser);
}
}  // namespace

TEST(UploadStream, BinaryExactEveryChunkSize)
{
    std::string data("\0\1\2\xff", 4);
    data += "\r\n--abcXYinside\r\n--ab\r\n--abcX\r\n";
    auto input = part("image", "a.png", data) + part("thumbnail", "a.jpg", "JPEG") + "--abc--\r\n";
    for (size_t chunk = 1; chunk <= 128; ++chunk) {
        Sink s;
        ASSERT_TRUE(run(s, input, chunk)) << chunk;
        ASSERT_EQ(s.payload.size(), 2u);
        EXPECT_EQ(s.payload[0], data);
        EXPECT_EQ(s.payload[1], "JPEG");
        EXPECT_EQ(s.ends, 2);
    }
}
TEST(UploadStream, EveryTwoChunkSplit)
{
    auto input = part("image", "a.png", std::string(1025, 'x')) + "--abc--";
    for (size_t cut = 0; cut <= input.size(); ++cut) {
        Sink s;
        upload_stream_t parser;
        auto io = s.io();
        ASSERT_TRUE(upload_stream_init(&parser, "abc", input.size(), &io));
        ASSERT_TRUE(upload_stream_feed(&parser, input.data(), cut));
        ASSERT_TRUE(upload_stream_feed(&parser, input.data() + cut, input.size() - cut));
        ASSERT_TRUE(upload_stream_finish(&parser));
        EXPECT_EQ(s.payload[0], std::string(1025, 'x'));
    }
}
TEST(UploadStream, BoundaryHeaderParsing)
{
    char b[71];
    EXPECT_TRUE(
        upload_boundary_from_content_type("multipart/form-data; boundary=abc", b, sizeof(b)));
    EXPECT_STREQ(b, "abc");
    EXPECT_TRUE(
        upload_boundary_from_content_type("Multipart/Form-Data; boundary=\"abc\"", b, sizeof(b)));
    for (const char *s :
         {"image/jpeg", "multipart/form-datax; boundary=abc", "multipart/form-data; boundary=",
          "multipart/form-data; boundary=\"abc", "multipart/form-data; boundary=abc; x=y",
          "multipart/form-data; boundary=abc\r\nBad", "multipart/form-data; boundary=\"a b\""})
        EXPECT_FALSE(upload_boundary_from_content_type(s, b, sizeof(b))) << s;
    EXPECT_FALSE(upload_boundary_from_content_type("multipart/form-data; boundary=abc", b, 3));
}
TEST(UploadStream, TraversalAndFatAliasesRejected)
{
    for (const char *s : {"", ".a", "..", "../a.png", "a/b.png", "a\\b.png", "C:a.png", "a?.png",
                          "a|.png", "a. ", "a.", "a\n.png"})
        EXPECT_FALSE(upload_leaf_safe(s)) << s;
    EXPECT_TRUE(upload_leaf_safe("原片 01.png"));
    EXPECT_FALSE(upload_leaf_safe(std::string(120, 'x').c_str()));
}
TEST(UploadStream, DuplicateAndUnknownFieldsRejected)
{
    for (const std::string &input :
         {part("image", "a.png", "x") + part("image", "b.png", "y") + "--abc--",
          part("unknown", "a.png", "x") + "--abc--"}) {
        Sink s;
        EXPECT_FALSE(run(s, input, 1));
    }
}
TEST(UploadStream, TruncatedAtEveryByteRejected)
{
    auto input = part("image", "a.png", "data") + "--abc--";
    for (size_t n = 0; n < input.size(); ++n) {
        Sink s;
        EXPECT_FALSE(run(s, input.substr(0, n), 13)) << n;
    }
}
TEST(UploadStream, TrailerMustBeCompleteAndNoJunk)
{
    auto input = part("image", "a.png", "data") + "--abc--";
    Sink a, b, c;
    EXPECT_FALSE(run(a, input + "\r"));
    EXPECT_FALSE(run(b, input + "junk"));
    EXPECT_TRUE(run(c, input + "\r\n"));
}
TEST(UploadStream, OverlongAndNulHeadersRejected)
{
    Sink a, b;
    EXPECT_FALSE(run(a, "--abc\r\n" + std::string(1025, 'a')));
    auto input = part("image", "a.png", "data") + "--abc--";
    input[10] = '\0';
    EXPECT_FALSE(run(b, input));
}
TEST(UploadStream, DuplicateDispositionAndParametersRejected)
{
    std::vector<std::string> headers = {
        "Content-Disposition: form-data; name=\"image\"; name=\"thumbnail\"; filename=\"a.png\"",
        "Content-Disposition: form-data; name=\"image\"; filename=\"a.png\"; filename=\"b.png\"",
        "Content-Disposition: form-data; name=\"image\"; "
        "filename=\"a.png\"\r\nContent-Disposition: form-data",
        "Content-Disposition: form-data; name=\"image\"; filename=\"a.png\"; bad=\"x\"",
        "Content-Disposition: form-data; name=image; filename=\"a.png\""};
    for (const auto &h : headers) {
        Sink s;
        EXPECT_FALSE(run(s, "--abc\r\n" + h + "\r\n\r\ndata\r\n--abc--"));
    }
}
TEST(UploadStream, SinkFailuresTerminal)
{
    auto input = part("image", "a.png", "data") + "--abc--";
    for (int fail = 0; fail < 3; ++fail) {
        Sink s;
        s.fail_begin = fail == 0;
        s.fail_write = fail == 1;
        s.fail_end = fail == 2;
        upload_stream_t parser;
        auto io = s.io();
        ASSERT_TRUE(upload_stream_init(&parser, "abc", 4096, &io));
        EXPECT_FALSE(upload_stream_feed(&parser, input.data(), input.size()));
        EXPECT_FALSE(upload_stream_feed(&parser, input.data(), input.size()));
        EXPECT_FALSE(upload_stream_finish(&parser));
    }
}
TEST(UploadStream, ByteLimitAndNullRejected)
{
    Sink s;
    auto input = part("image", "a.png", "data") + "--abc--";
    EXPECT_FALSE(run(s, input, 4096, input.size() - 1));
    upload_stream_t parser;
    auto io = s.io();
    ASSERT_TRUE(upload_stream_init(&parser, "abc", 4096, &io));
    EXPECT_FALSE(upload_stream_feed(&parser, nullptr, 1));
    EXPECT_FALSE(upload_stream_init(&parser, std::string(71, 'a').c_str(), 4096, &io));
}
