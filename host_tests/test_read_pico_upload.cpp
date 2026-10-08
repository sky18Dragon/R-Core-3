#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <map>
#include <string>

#include "read_pico_sd.h"
#include "read_pico_upload.h"
#include "upload_probe.h"

namespace
{
std::map<std::string, std::string> files;
std::map<int, std::string> handles;
int opens, closes, syncs, renames, close_failures, next_fd, collisions;
bool media, fail_close, fail_sync;
bool allow_overwrite;
int fail_read_call, read_calls;
std::map<int, size_t> cursors;
std::string last_path;
}  // namespace
extern "C" {
uint32_t esp_random(void)
{
    return 0x12345678;
}
esp_err_t read_pico_sd_get_status(read_pico_sd_status_t *s)
{
    *s = {};
    s->present = s->mounted = s->media_valid = media;
    return ESP_OK;
}
void read_pico_sd_note_file_close_failure(void)
{
    ++close_failures;
    media = false;
}
int upload_test_open(const char *path, int flags, ...)
{
    ++opens;
    last_path = path;
    EXPECT_EQ(flags, O_WRONLY | O_CREAT | O_EXCL);
    va_list args;
    va_start(args, flags);
    EXPECT_EQ(va_arg(args, int), 0600);
    va_end(args);
    if (collisions-- > 0 || files.count(path)) {
        errno = EEXIST;
        return -1;
    }
    files[path] = "";
    int fd = next_fd++;
    handles[fd] = path;
    return fd;
}
ssize_t upload_test_write(int fd, const void *p, size_t n)
{
    if (!handles.count(fd)) {
        errno = EBADF;
        return -1;
    }
    files[handles.at(fd)].append(static_cast<const char *>(p), n);
    return n;
}
int upload_test_fsync(int fd)
{
    EXPECT_EQ(handles.count(fd), 1u);
    ++syncs;
    return fail_sync ? -1 : 0;
}
int upload_test_close(int fd)
{
    ++closes;
    if (fail_close) {
        errno = EIO;
        return -1;
    }
    return handles.erase(fd) ? 0 : -1;
}
int upload_test_rename(const char *a, const char *b)
{
    ++renames;
    // Explicit FAT-like no-replace substitute, not host POSIX rename.
    if (files.count(b) && !allow_overwrite) {
        errno = EEXIST;
        return -1;
    }
    if (!files.count(a)) {
        errno = ENOENT;
        return -1;
    }
    files[b] = files[a];
    files.erase(a);
    return 0;
}
}
namespace
{
bool target_absent(void *)
{
    return !files.count(UPLOAD_PROBE_TARGET);
}
int open_read(void *, const char *path)
{
    if (!files.count(path))
        return -1;
    int fd = next_fd++;
    handles[fd] = path;
    cursors[fd] = 0;
    return fd;
}
int read_file(void *, int fd, uint8_t *p, size_t n)
{
    if (++read_calls == fail_read_call)
        return -1;
    auto &data = files.at(handles.at(fd));
    size_t &cursor = cursors[fd];
    size_t count = std::min(n, data.size() - cursor);
    memcpy(p, data.data() + cursor, count);
    cursor += count;
    return static_cast<int>(count);
}
upload_probe_io_t probe_io()
{
    return {read_pico_upload_io(), target_absent, open_read, read_file};
}
}  // namespace
class ReadPicoUpload : public testing::Test
{
   protected:
    void SetUp() override
    {
        files.clear();
        handles.clear();
        opens = closes = syncs = renames = close_failures = next_fd = collisions = 0;
        media = true;
        fail_close = fail_sync = false;
        last_path.clear();
        allow_overwrite = false;
        cursors.clear();
        read_calls = fail_read_call = 0;
    }
    bool begin(upload_file_t &f, char *path)
    {
        return read_pico_upload_begin(&f, "/storage/images/Default", path, 512, 4096);
    }
    bool finish(upload_file_t &f)
    {
        return upload_file_write(&f, reinterpret_cast<const uint8_t *>("data"), 4) &&
               upload_file_finish(&f, 4);
    }
};
TEST_F(ReadPicoUpload, ExclusiveHiddenDistinctFiles)
{
    upload_file_t a, b;
    char p[512], q[512];
    ASSERT_TRUE(begin(a, p));
    ASSERT_TRUE(begin(b, q));
    EXPECT_NE(std::string(p), q);
    EXPECT_NE(std::string(p).find("/.rc3-upload-"), std::string::npos);
    EXPECT_EQ(std::string(p).substr(std::string(p).size() - 5), ".part");
    ASSERT_TRUE(finish(a));
    ASSERT_TRUE(finish(b));
    EXPECT_EQ(files.size(), 2u);
    EXPECT_TRUE(handles.empty());
}
TEST_F(ReadPicoUpload, BoundedCollisionRetryNeverTruncates)
{
    collisions = 2;
    upload_file_t a;
    char p[512];
    ASSERT_TRUE(begin(a, p));
    EXPECT_EQ(opens, 3);
    ASSERT_TRUE(finish(a));
    files["/storage/images/Default/original.png"] = "ORIGINAL";
    EXPECT_FALSE(upload_file_publish(&a, p, "/storage/images/Default/original.png"));
    EXPECT_EQ(files[p], "data");
    EXPECT_EQ(files["/storage/images/Default/original.png"], "ORIGINAL");
}
TEST_F(ReadPicoUpload, RetryExhaustionDoesNotWrite)
{
    collisions = 100;
    upload_file_t a;
    char p[512];
    EXPECT_FALSE(begin(a, p));
    EXPECT_EQ(opens, 16);
    EXPECT_TRUE(files.empty());
    EXPECT_EQ(closes, 0);
}
TEST_F(ReadPicoUpload, InvalidDirectoryAndTruncationNoCreate)
{
    for (const char *dir :
         {"/storage-other", "/sdcard", "/storage/..", "/storage/../x", "/storage/./x",
          "/storage/x/", "/storage/x\\y", "/storage/x\n", "/storage/x:y"}) {
        upload_file_t f;
        char p[512];
        EXPECT_FALSE(read_pico_upload_begin(&f, dir, p, sizeof(p), 4096)) << dir;
    }
    upload_file_t f;
    char p[8];
    EXPECT_FALSE(read_pico_upload_begin(&f, "/storage", p, sizeof(p), 4096));
    EXPECT_EQ(opens, 0);
}
TEST_F(ReadPicoUpload, InvalidPublicationNoRename)
{
    for (const char *to : {"/other/a", "/storage/../a", "/storage/./a", "/storage"}) {
        upload_file_t f;
        char p[512];
        ASSERT_TRUE(begin(f, p));
        ASSERT_TRUE(finish(f));
        EXPECT_FALSE(upload_file_publish(&f, p, to));
    }
    EXPECT_EQ(renames, 0);
}
TEST_F(ReadPicoUpload, SyncOrCloseFailureRetainsTemporary)
{
    for (int fail = 0; fail < 2; ++fail) {
        SetUp();
        fail_sync = fail == 0;
        fail_close = fail == 1;
        upload_file_t f;
        char p[512];
        ASSERT_TRUE(begin(f, p));
        ASSERT_TRUE(upload_file_write(&f, reinterpret_cast<const uint8_t *>("data"), 4));
        EXPECT_FALSE(upload_file_finish(&f, 4));
        upload_file_abort(&f);
        EXPECT_EQ(closes, 1);
        EXPECT_EQ(close_failures, fail == 1 ? 1 : 0);
        EXPECT_EQ(files[p], "data");
        EXPECT_FALSE(upload_file_publish(&f, p, "/storage/images/Default/new.png"));
        EXPECT_EQ(renames, 0);
    }
}
TEST_F(ReadPicoUpload, ClosedCompleteFilePublishesWithoutDeletion)
{
    upload_file_t f;
    char p[512];
    ASSERT_TRUE(begin(f, p));
    ASSERT_TRUE(finish(f));
    files["/storage/images/Default/other.png"] = "OTHER";
    ASSERT_TRUE(upload_file_publish(&f, p, "/storage/images/Default/new.png"));
    EXPECT_EQ(files["/storage/images/Default/new.png"], "data");
    EXPECT_EQ(files["/storage/images/Default/other.png"], "OTHER");
    EXPECT_EQ(renames, 1);
    EXPECT_EQ(syncs, 1);
}
TEST_F(ReadPicoUpload, ProbeSuccessAndOneShot)
{
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.created, 2u);
    EXPECT_EQ(result.written, 4112u);
    EXPECT_EQ(result.published, 1u);
    EXPECT_EQ(result.verified, 8192u);
    EXPECT_TRUE(result.conflict_rejected);
    EXPECT_EQ(files.size(), 2u);
    EXPECT_EQ(files.at(UPLOAD_PROBE_TARGET).size(), 4096u);
    EXPECT_EQ(files.at(UPLOAD_PROBE_CONFLICT), std::string(16, '\xA5'));
    EXPECT_FALSE(session.close_failed);
    EXPECT_TRUE(handles.empty());
    int before = opens;
    EXPECT_FALSE(upload_probe_run(&session, &io).ok);
    EXPECT_EQ(opens, before);
}
TEST_F(ReadPicoUpload, ProbeExistingTargetPreflightNoWrites)
{
    files[UPLOAD_PROBE_TARGET] = "ORIGINAL";
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(opens, 0);
    EXPECT_EQ(files[UPLOAD_PROBE_TARGET], "ORIGINAL");
    EXPECT_EQ(files.size(), 1u);
}
TEST_F(ReadPicoUpload, ProbeExistingStageRetained)
{
    files[UPLOAD_PROBE_STAGE] = "ORIGINAL";
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.created, 0u);
    EXPECT_EQ(files[UPLOAD_PROBE_STAGE], "ORIGINAL");
    EXPECT_EQ(files.size(), 1u);
}
TEST_F(ReadPicoUpload, ProbeExistingConflictStageStopsWithoutDelete)
{
    files[UPLOAD_PROBE_CONFLICT] = "ORIGINAL";
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.published, 1u);
    EXPECT_EQ(files[UPLOAD_PROBE_CONFLICT], "ORIGINAL");
    EXPECT_EQ(files[UPLOAD_PROBE_TARGET].size(), 4096u);
}
TEST_F(ReadPicoUpload, ProbeUnsafeRenameDetectedNotAccepted)
{
    allow_overwrite = true;
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_STREQ(result.stage, "conflict");
    EXPECT_FALSE(result.conflict_rejected);  // Host-only fault injection, never on original files.
}
TEST_F(ReadPicoUpload, ProbeCloseFailureLatchesAndDoesNotRetry)
{
    fail_close = true;
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(closes, 1);
    EXPECT_EQ(renames, 0);
    EXPECT_EQ(files.size(), 1u);
}
TEST_F(ReadPicoUpload, ProbeReadFailureClosesAndRetainsPublishedFile)
{
    fail_read_call = 3;
    upload_probe_session_t session = {};
    auto io = probe_io();
    auto result = upload_probe_run(&session, &io);
    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(handles.empty());
    EXPECT_EQ(result.published, 1u);
    EXPECT_EQ(files[UPLOAD_PROBE_TARGET].size(), 4096u);
}
