#include <gtest/gtest.h>
#include <cerrno>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "upload_asset.h"
#include "download_file.h"
#include "upload_copy.h"
#include "text_buffer.h"

TEST(TextBuffer, EveryCapacityTerminatesWithoutTouchingGuards) {
    for (size_t capacity = 0; capacity <= 20; ++capacity) {
        char storage[22]; memset(storage, '!', sizeof(storage));
        text_buffer_message(storage + 1, capacity, "diagnostic");
        EXPECT_EQ(storage[0], '!');
        EXPECT_EQ(storage[capacity + 1], '!');
        if (capacity) {
            size_t length = std::min(capacity - 1, size_t(10));
            EXPECT_EQ(memcmp(storage + 1, "diagnostic", length), 0);
            EXPECT_EQ(storage[length + 1], 0);
        } else EXPECT_EQ(storage[1], '!');
    }
}
TEST(TextBuffer, OptionalOutputAndEmptyMessageAreSafe) {
    text_buffer_message(nullptr, 0, "message");
    text_buffer_message(nullptr, 8, "message");
    char out[2] = {'!', '!'};
    text_buffer_message(out, 0, nullptr); EXPECT_EQ(out[0], '!');
    text_buffer_message(out, sizeof(out), nullptr); EXPECT_EQ(out[0], 0); EXPECT_EQ(out[1], '!');
    out[0] = '!'; text_buffer_message(out, sizeof(out), ""); EXPECT_EQ(out[0], 0);
}
TEST(TextBuffer, ProtocolCopyRejectsOverlengthWithoutPublishingPrefix) {
    for (size_t capacity : {size_t(1), size_t(128), size_t(512)}) {
        std::vector<char> output(capacity + 2, '!');
        std::string fits(capacity - 1, 'a'), exceeds(capacity, 'b');
        ASSERT_TRUE(text_buffer_copy_exact(output.data() + 1, capacity, fits.c_str()));
        EXPECT_STREQ(output.data() + 1, fits.c_str());
        auto before = output;
        EXPECT_FALSE(text_buffer_copy_exact(output.data() + 1, capacity, exceeds.c_str()));
        EXPECT_EQ(output, before);
        EXPECT_FALSE(text_buffer_copy_exact(output.data() + 1, 0, ""));
        EXPECT_FALSE(text_buffer_copy_exact(output.data() + 1, capacity, nullptr));
        EXPECT_EQ(output, before);
        EXPECT_EQ(output.front(), '!'); EXPECT_EQ(output.back(), '!');
    }
    EXPECT_FALSE(text_buffer_copy_exact(nullptr, 512, "value"));
}
TEST(TextBuffer, SingletonCannotBeReplacedAfterAcceptedOrInvalidValue) {
    EXPECT_FALSE(text_buffer_claim_once(nullptr));
    for (const char *value : {"ok", "oversize"}) {
        bool seen = false; char out[4] = "old";
        ASSERT_TRUE(text_buffer_claim_once(&seen));
        bool copied = text_buffer_copy_exact(out, sizeof(out), value);
        EXPECT_EQ(copied, !strcmp(value, "ok"));
        std::string before(out);
        EXPECT_FALSE(text_buffer_claim_once(&seen));
        EXPECT_STREQ(out, before.c_str());
        seen = false; // Only a new response resets the singleton state.
        EXPECT_TRUE(text_buffer_claim_once(&seen));
    }
}
TEST(DownloadOrigin, AbsoluteAndRelativeReferencesRejectEveryUnsafeByte) {
    for (unsigned byte = 1; byte <= 127; ++byte) {
        if (byte > 32 && byte != 127 && byte != '\\') continue;
        for (const char *prefix : {"https://host/", "https://host/?q=", "https://host/#"}) {
            std::string bad = std::string(prefix) + char(byte) + "suffix";
            EXPECT_FALSE(download_same_origin("https://host/", bad.c_str()));
            EXPECT_FALSE(download_same_origin(bad.c_str(), "https://host/"));
            EXPECT_FALSE(download_redirect_same_origin("https://host/", bad.c_str()));
            EXPECT_FALSE(download_redirect_same_origin(bad.c_str(), "/safe"));
        }
        std::string relative = std::string("/path") + char(byte) + "suffix";
        EXPECT_FALSE(download_redirect_same_origin("https://host/", relative.c_str()));
    }
    EXPECT_TRUE(download_same_origin("https://host/path%20name", "https://HOST/?q=%0A"));
}

namespace {
constexpr const char *source = "/storage/.rc3-upload-12345678-00000001.part";
constexpr const char *thumbnail = "/storage/.rc3-upload-abcdef01-00000002.part";
struct FS {
    std::map<std::string, std::string> objects;
    std::map<int, std::string> handles;
    std::vector<std::string> published;
    bool media = true, lose_after_publish = false;
    int next = 0, fail_publish = 0;
    static FS &get(void *p) { return *static_cast<FS *>(p); }
    static bool ready(void *p) { return get(p).media; }
    static int create(void *p, const char *path) {
        auto &s = get(p);
        if (s.objects.count(path)) { errno = EEXIST; return -1; }
        s.objects[path] = "";
        int fd = s.next++;
        s.handles[fd] = path;
        return fd;
    }
    static int write(void *p, int fd, const uint8_t *bytes, size_t n) {
        auto &s = get(p);
        s.objects[s.handles.at(fd)].append(reinterpret_cast<const char *>(bytes), n);
        return static_cast<int>(n);
    }
    static int sync(void *, int) { return 0; }
    static int close(void *p, int fd) { get(p).handles.erase(fd); return 0; }
    static void uncertain(void *) { ADD_FAILURE(); }
    static int rename(void *p, const char *from, const char *to) {
        auto &s = get(p);
        EXPECT_TRUE(s.handles.empty()); // Every owner must already be closed.
        s.published.emplace_back(to);
        if (s.fail_publish == static_cast<int>(s.published.size())) return -1;
        if (s.objects.count(to)) { errno = EEXIST; return -1; }
        s.objects[to] = s.objects.at(from);
        s.objects.erase(from);
        if (s.lose_after_publish) s.media = false;
        return 0;
    }
    upload_file_t complete(const char *path, const std::string &data = "image") {
        upload_file_io_t io = {this, ready, create, write, sync, close, uncertain, rename};
        upload_file_t f = {};
        EXPECT_TRUE(upload_file_begin(&f, &io, path, 4096));
        EXPECT_TRUE(upload_file_write(&f, reinterpret_cast<const uint8_t *>(data.data()), data.size()));
        EXPECT_TRUE(upload_file_finish(&f, data.size()));
        return f;
    }
};
}
TEST(UploadAsset, AllSupportedTypesBecomeIndependentImmutableNames) {
    for (const char *ext : {".jpg", ".png", ".bmp", ".epdgz"}) {
        FS s; s.objects["/storage/.current.jpg"] = "previous";
        auto f = s.complete(source); upload_asset_t out;
        ASSERT_TRUE(upload_asset_publish(&f, nullptr, ext, &out));
        EXPECT_TRUE(out.image_published); EXPECT_FALSE(out.preview_published);
        EXPECT_TRUE(upload_asset_is_typed(out.image));
        EXPECT_EQ(s.objects.at(out.image), "image");
        EXPECT_EQ(s.objects.at("/storage/.current.jpg"), "previous");
        EXPECT_FALSE(s.objects.count(source));
    }
}
TEST(UploadAsset, PreviewPublishesFirstToDedicatedSiblingEvenForJpeg) {
    FS s; auto f = s.complete(source), t = s.complete(thumbnail, "preview"); upload_asset_t out;
    ASSERT_TRUE(upload_asset_publish(&f, &t, ".jpg", &out));
    ASSERT_EQ(s.published.size(), 2u);
    EXPECT_EQ(s.published[0], out.preview); EXPECT_EQ(s.published[1], out.image);
    EXPECT_NE(std::string(out.image), out.preview);
    EXPECT_EQ(s.objects.at(out.image), "image"); EXPECT_EQ(s.objects.at(out.preview), "preview");
}
TEST(UploadAsset, ExistingPreviewNeverOverwrittenAndMainNotPublished) {
    FS s; auto f = s.complete(source), t = s.complete(thumbnail); upload_asset_t out;
    ASSERT_TRUE(upload_asset_plan(&f, &t, ".png", &out));
    s.objects[out.preview] = "original";
    EXPECT_FALSE(upload_asset_publish(&f, &t, ".png", &out));
    EXPECT_FALSE(out.image_published); EXPECT_FALSE(out.preview_published);
    EXPECT_EQ(s.objects.at(out.preview), "original");
    EXPECT_TRUE(s.objects.count(source)); EXPECT_TRUE(s.objects.count(thumbnail));
}
TEST(UploadAsset, ExistingImageNeverOverwrittenPartialPreviewRetained) {
    FS s; auto f = s.complete(source), t = s.complete(thumbnail); upload_asset_t out;
    ASSERT_TRUE(upload_asset_plan(&f, &t, ".png", &out)); s.objects[out.image] = "original";
    EXPECT_FALSE(upload_asset_publish(&f, &t, ".png", &out));
    EXPECT_TRUE(out.preview_published); EXPECT_FALSE(out.image_published);
    EXPECT_EQ(s.objects.at(out.image), "original"); EXPECT_TRUE(s.objects.count(source));
    size_t calls = s.published.size();
    EXPECT_FALSE(upload_asset_publish(&f, &t, ".png", &out)); EXPECT_EQ(s.published.size(), calls);
}
TEST(UploadAsset, UnfinishedOrFailedOwnersCannotPlanOrPublish) {
    for (int fault = 0; fault < 6; ++fault) {
        FS s; auto f = s.complete(source); upload_asset_t out;
        if (fault == 0) f.ready = false;
        if (fault == 1) f.fd = 2;
        if (fault == 2) f.failed = true;
        if (fault == 3) f.close_failed = true;
        if (fault == 4) f.published = true;
        if (fault == 5) f.bytes = 0;
        EXPECT_FALSE(upload_asset_plan(&f, nullptr, ".jpg", &out));
        EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", &out));
        EXPECT_TRUE(s.published.empty()); EXPECT_TRUE(s.objects.count(source));
    }
}
TEST(UploadAsset, CallerOriginalNamesNeverAccepted) {
    for (const char *path : {"/storage/original.jpg", "/storage/.current.tmp",
                            "/storage/.rc3-upload-x.part", "/storage/.rc3-upload-12345678-00000001.jpg",
                            "/storage/.rc3-upload-12345678_00000001.part"}) {
        FS s; auto f = s.complete(path); upload_asset_t out;
        EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", &out));
        EXPECT_TRUE(s.published.empty()); EXPECT_EQ(s.objects.at(path), "image");
    }
}
TEST(UploadAsset, AliasedOrUnfinishedPreviewRejectedBeforeAnyPublish) {
    FS s; auto f = s.complete(source), t = s.complete(thumbnail); upload_asset_t out;
    EXPECT_FALSE(upload_asset_publish(&f, &f, ".png", &out));
    t.ready = false; EXPECT_FALSE(upload_asset_publish(&f, &t, ".png", &out));
    t.ready = true; strcpy(t.source, source);
    EXPECT_FALSE(upload_asset_publish(&f, &t, ".png", &out)); EXPECT_TRUE(s.published.empty());
}
TEST(UploadAsset, InvalidExtensionAndOutputBoundaries) {
    FS s; auto f = s.complete(source); upload_asset_t out;
    for (const char *ext : {".jpeg", ".part", ".JPG", "", "../x"})
        EXPECT_FALSE(upload_asset_publish(&f, nullptr, ext, &out));
    EXPECT_FALSE(upload_asset_publish(&f, nullptr, nullptr, &out));
    EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", nullptr));
    char tiny[4]; EXPECT_FALSE(upload_asset_preview_path("/storage/original.jpg", tiny, sizeof(tiny)));
    EXPECT_FALSE(upload_asset_preview_path("/storage/.rc3-upload-12345678-00000001.jpg", tiny, sizeof(tiny)));
    EXPECT_TRUE(s.published.empty());
}
TEST(UploadAsset, MediaLostBeforePublicationMakesNoRename) {
    FS s; auto f = s.complete(source); s.media = false; upload_asset_t out;
    EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", &out));
    EXPECT_TRUE(s.published.empty()); EXPECT_TRUE(s.objects.count(source));
}
TEST(UploadAsset, MediaLostAfterPreviewRenameReportsPartialStateAndNoMain) {
    FS s; auto f = s.complete(source), t = s.complete(thumbnail); s.lose_after_publish = true;
    upload_asset_t out; EXPECT_FALSE(upload_asset_publish(&f, &t, ".jpg", &out));
    EXPECT_TRUE(out.preview_published); EXPECT_FALSE(out.image_published);
    EXPECT_TRUE(s.objects.count(out.preview)); EXPECT_TRUE(s.objects.count(source));
}
TEST(UploadAsset, MediaLostAfterMainRenameCannotClaimNoPublicationOrRetry) {
    FS s; auto f = s.complete(source); s.lose_after_publish = true; upload_asset_t out;
    EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", &out));
    EXPECT_TRUE(out.image_published); EXPECT_TRUE(s.objects.count(out.image));
    EXPECT_FALSE(upload_asset_publish(&f, nullptr, ".jpg", &out)); EXPECT_EQ(s.published.size(), 1u);
}
TEST(UploadAsset, MalformedShortIdentifiersAndLongSourcesAreBounded) {
    char output[512];
    for (unsigned n = 0; n < strlen(source); ++n) {
        std::string part(source, n);
        EXPECT_FALSE(upload_asset_is_typed(part.c_str()));
        EXPECT_FALSE(upload_asset_preview_path(part.c_str(), output, sizeof(output)));
    }
    std::string huge(512, 'x'); EXPECT_FALSE(upload_asset_is_typed(huge.c_str()));
    EXPECT_FALSE(upload_asset_is_typed(nullptr));
    EXPECT_FALSE(upload_asset_preview_path("/storage/.rc3-upload-12345678-00000001.jpg", nullptr, 512));
}

TEST(DownloadFile, RedirectAndErrorBodiesNeverEnterFinalImage) {
    FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                               FS::close, FS::uncertain, FS::rename};
    upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 32));
    EXPECT_TRUE(download_file_write(&f, 302, "bad", 3, 0));
    EXPECT_TRUE(download_file_write(&f, 500, "error", 5, 1));
    EXPECT_TRUE(download_file_write(&f, 200, "image", 5, 2));
    EXPECT_EQ(download_file_finish(&f, true, 200, true, 5), DOWNLOAD_COMPLETE);
    EXPECT_EQ(s.objects.at(source), "image"); EXPECT_TRUE(s.handles.empty());
}
TEST(DownloadFile, IncompleteTransportLengthsAndStatusesCannotClaimSuccess) {
    for (int which = 0; which < 7; ++which) {
        FS s; auto f = s.complete(source); f.ready = false;
        f.fd = FS::create(&s, "/storage/new");
        EXPECT_EQ(download_file_finish(&f, which != 0, which == 1 ? 500 : 200,
                                      which != 2, which == 3 ? 4 : which == 4 ? 6 :
                                      which == 5 ? -2 : 0), DOWNLOAD_FAILED);
        EXPECT_TRUE(s.handles.empty()); EXPECT_FALSE(f.ready);
        EXPECT_TRUE(s.objects.count(source)); EXPECT_TRUE(s.published.empty());
    }
}
TEST(DownloadFile, ChunkedCompleteAllowsUnknownLengthButIncompleteDoesNot) {
    for (bool complete : {false, true}) {
        FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                                   FS::close, FS::uncertain, FS::rename};
        upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 32));
        ASSERT_TRUE(download_file_write(&f, 200, "image", 5, 0));
        EXPECT_EQ(download_file_finish(&f, true, 200, complete, -1),
                  complete ? DOWNLOAD_COMPLETE : DOWNLOAD_FAILED);
    }
}
TEST(DownloadFile, CallbackFailureStaysLatchedEvenWhenHttpPerformReportsSuccess) {
    for (int which = 0; which < 4; ++which) {
        FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                                   FS::close, FS::uncertain, FS::rename};
        upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 4));
        if (which == 3) s.media = false;
        EXPECT_FALSE(download_file_write(&f, 200, "image", which == 0 ? -1 :
                                         which == 2 ? 5 : 3, which == 1 ? 120000 : 0));
        EXPECT_EQ(download_file_finish(&f, true, 200, true, -1), DOWNLOAD_FAILED);
        EXPECT_TRUE(s.handles.empty()); EXPECT_FALSE(f.ready);
    }
}
TEST(DownloadFile, NotModifiedClosesEmptyStageButNeverPublishesOrDeletes) {
    FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                               FS::close, FS::uncertain, FS::rename};
    upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 32));
    EXPECT_TRUE(download_file_write(&f, 304, "ignored", 7, 0));
    EXPECT_EQ(download_file_finish(&f, true, 304, false, -1), DOWNLOAD_NOT_MODIFIED);
    EXPECT_TRUE(s.handles.empty()); EXPECT_TRUE(s.objects.count(source));
    EXPECT_TRUE(s.objects.at(source).empty()); EXPECT_FALSE(f.ready);
    EXPECT_EQ(download_file_finish(nullptr, true, 200, true, 5), DOWNLOAD_FAILED);
}
TEST(DownloadOrigin, NormalizedDefaultPortsHostCaseAndIpv6) {
    EXPECT_TRUE(download_same_origin("https://HOST/image", "https://host:443/preview"));
    EXPECT_TRUE(download_same_origin("http://host:80/a", "http://HOST/b?q=x"));
    EXPECT_TRUE(download_same_origin("http://[::1]:8080/a", "HTTP://[::1]:8080/b"));
    EXPECT_FALSE(download_same_origin("https://host/a", "http://host/b"));
    EXPECT_FALSE(download_same_origin("https://host:444/a", "https://host/b"));
    EXPECT_FALSE(download_same_origin("https://host/a", "https://host.evil/b"));
    for (const char *target : {"/image", "../image", "preview.jpg", "?v=2", "https://HOST:443/x"})
        EXPECT_TRUE(download_redirect_same_origin("https://host/a", target));
    for (const char *target : {"//evil/x", "https://evil/x", "http://host/x", "ftp:bad", "\\evil", ""})
        EXPECT_FALSE(download_redirect_same_origin("https://host/a", target));
}
TEST(DownloadOrigin, UnknownUserinfoMalformedAndOversizeUrlsNeverReceiveCredentials) {
    for (const char *url : {"/preview", "//host/x", "ftp://host/a", "https:///a",
                           "https://user@host/a", "https://host:/a", "https://host:999999999/a",
                           "https://host:0/a", "https://host:abc/a", "https://[::1/a",
                           "https://[::1]oops/a", "https://ho\\st/a", "https://ho st/a"})
        EXPECT_FALSE(download_same_origin("https://host/a", url)) << url;
    EXPECT_FALSE(download_same_origin(nullptr, "https://host/a"));
    std::string huge = "https://" + std::string(2048, 'a');
    EXPECT_FALSE(download_same_origin(huge.c_str(), huge.c_str()));
    EXPECT_FALSE(download_redirect_same_origin("https://host/a", huge.c_str()));
    EXPECT_FALSE(download_redirect_same_origin(nullptr, "/x"));
}

namespace {
struct CopySource {
    std::string content; size_t at = 0; int closes = 0, reads = 0;
    bool short_read = false, close_failure = false;
    static int read(void *c, uint8_t *out, size_t cap) {
        auto &s = *static_cast<CopySource *>(c); ++s.reads;
        size_t n = std::min(cap, s.content.size() - s.at);
        if (s.short_read && n) --n;
        memcpy(out, s.content.data() + s.at, n); s.at += n; return (int)n;
    }
    static int close(void *c) {
        auto &s = *static_cast<CopySource *>(c); ++s.closes; return s.close_failure ? -1 : 0;
    }
};
}
TEST(UploadCopy, ExactLengthEofClosedBeforeDestinationSyncAndImmutablePublication) {
    FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                               FS::close, FS::uncertain, FS::rename};
    upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 4096));
    CopySource input{std::string(2049, 'a')};
    upload_copy_source_t reader{&input, CopySource::read, CopySource::close};
    ASSERT_TRUE(upload_copy_finish(&f, &reader, input.content.size()));
    EXPECT_EQ(input.closes, 1); EXPECT_EQ(input.reads, 10); EXPECT_TRUE(s.handles.empty());
    EXPECT_EQ(s.objects.at(source), input.content);
    ASSERT_TRUE(upload_file_publish(&f, source, "/storage/album.jpg"));
}
TEST(UploadCopy, ShortReadTrailingDataZeroLengthAndLimitFailWithoutPublication) {
    for (int which = 0; which < 4; ++which) {
        FS s; upload_file_io_t io = {&s, FS::ready, FS::create, FS::write, FS::sync,
                                   FS::close, FS::uncertain, FS::rename};
        upload_file_t f; ASSERT_TRUE(upload_file_begin(&f, &io, source, 32));
        CopySource input{"abcde"}; input.short_read = which == 0;
        upload_copy_source_t reader{&input, CopySource::read, CopySource::close};
        EXPECT_FALSE(upload_copy_finish(&f, &reader, which == 1 ? 4 : which == 2 ? 0 :
                                       which == 3 ? 33 : 5));
        EXPECT_EQ(input.closes, 1); EXPECT_TRUE(s.handles.empty());
        EXPECT_TRUE(s.objects.count(source)); EXPECT_FALSE(f.ready);
    }
}
