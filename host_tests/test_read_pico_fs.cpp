#include <gtest/gtest.h>

#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>

extern "C" {
#include "read_pico_fs.h"
#include "read_pico_sd.h"
}

namespace {
std::mutex registry_mutex;
thread_local unsigned registry_depth;
std::atomic<unsigned> users, begins, ends, real_opens, real_closes;
std::atomic<bool> poisoned;
esp_err_t begin_result;
bool open_fail, close_fail, fdopen_fail, metadata_fail, observe_pending;
bool same_handle, reuse_during_close;
const char *reuse_path;
int last_flags, last_mode;
std::string directory;
}

extern "C" {
void read_pico_fs_test_lock(void) { registry_mutex.lock(); ++registry_depth; }
void read_pico_fs_test_unlock(void) { --registry_depth; registry_mutex.unlock(); }
esp_err_t read_pico_sd_io_begin(void)
{
    EXPECT_EQ(registry_depth, 0u);
    ++begins;
    if (poisoned)
        return ESP_ERR_INVALID_STATE;
    if (begin_result != ESP_OK)
        return begin_result;
    ++users;
    return ESP_OK;
}
void read_pico_sd_io_end(void)
{
    EXPECT_EQ(registry_depth, 0u);
    EXPECT_GT(users.load(), 0u);
    --users;
    ++ends;
}
void read_pico_sd_note_file_close_failure(void) { poisoned = true; }
FILE *__real_fopen(const char *, const char *)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_opens;
    if (observe_pending)
        EXPECT_GT(users.load(), 0u); // Opening itself pins the mount.
    if (open_fail) { errno = ENOENT; return nullptr; }
    if (same_handle)
        return reinterpret_cast<FILE *>(uintptr_t(0x1234)); // Opaque test key, never dereferenced.
    return tmpfile();
}
FILE *__real_fdopen(int fd, const char *mode)
{
    EXPECT_EQ(registry_depth, 0u);
    if (fdopen_fail) { errno = EINVAL; return nullptr; }
    return fdopen(fd, mode);
}
int __real_fclose(FILE *file)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_closes;
    if (observe_pending)
        EXPECT_GT(users.load(), 0u); // Close/flush must finish before releasing.
    if (same_handle) {
        if (reuse_during_close) {
            reuse_during_close = false;
            FILE *next = __wrap_fopen(reuse_path, "rb");
            EXPECT_EQ(next, file);
            EXPECT_EQ(users.load(), reuse_path[1] == 's' ? 2u : 1u);
            EXPECT_EQ(__wrap_fclose(next), 0);
        }
        return 0;
    }
    int result = fclose(file);
    if (close_fail) { errno = EIO; return EOF; }
    return result;
}
int __real_open(const char *, int flags, ...)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_opens;
    last_flags = flags;
    va_list args;
    va_start(args, flags);
    last_mode = va_arg(args, int);
    va_end(args);
    if (observe_pending)
        EXPECT_GT(users.load(), 0u);
    if (open_fail) { errno = ENOENT; return -1; }
    char path[] = "/tmp/read-pico-fd-XXXXXX";
    int fd = mkstemp(path);
    if (fd >= 0)
        unlink(path); // Test-owned anonymous file only; never a real SD path.
    return fd;
}
int __real_close(int fd)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_closes;
    if (observe_pending)
        EXPECT_GT(users.load(), 0u);
    int result = close(fd);
    if (close_fail) { errno = EIO; return -1; }
    return result;
}
DIR *__real_opendir(const char *)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_opens;
    if (open_fail) { errno = ENOENT; return nullptr; }
    return opendir(directory.c_str());
}
int __real_closedir(DIR *dir)
{
    EXPECT_EQ(registry_depth, 0u);
    ++real_closes;
    if (observe_pending)
        EXPECT_GT(users.load(), 0u);
    int result = closedir(dir);
    if (close_fail) { errno = EIO; return -1; }
    return result;
}
int __real_stat(const char *, struct stat *)
{
    EXPECT_EQ(registry_depth, 0u);
    if (observe_pending)
        EXPECT_GT(users.load(), 0u);
    if (metadata_fail) { errno = ENOENT; return -1; }
    return 0;
}
int __real_mkdir(const char *, mode_t) { return __real_stat(nullptr, nullptr); }
int __real_unlink(const char *) { return __real_stat(nullptr, nullptr); }
int __real_rename(const char *, const char *) { return __real_stat(nullptr, nullptr); }
}

class ReadPicoFs : public testing::Test {
protected:
    void SetUp() override
    {
        users = begins = ends = real_opens = real_closes = 0;
        poisoned = false;
        begin_result = ESP_OK;
        open_fail = close_fail = fdopen_fail = metadata_fail = observe_pending = false;
        same_handle = reuse_during_close = false;
        reuse_path = "/storage/next.jpg";
        last_flags = last_mode = 0;
        char path[] = "/tmp/read-pico-dir-XXXXXX";
        ASSERT_NE(mkdtemp(path), nullptr);
        directory = path;
    }
    void TearDown() override
    {
        EXPECT_EQ(registry_depth, 0u);
        if (!poisoned)
            EXPECT_EQ(users.load(), 0u);
        EXPECT_EQ(rmdir(directory.c_str()), 0);
    }
};

TEST_F(ReadPicoFs, StorageFilePinsDuringOpenAndCloseThenReleasesExactlyOnce)
{
    observe_pending = true;
    FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(users.load(), 1u);
    EXPECT_EQ(__wrap_fclose(file), 0);
    EXPECT_EQ(users.load(), 0u);
    EXPECT_EQ(begins.load(), 1u);
    EXPECT_EQ(ends.load(), 1u);
}
TEST_F(ReadPicoFs, OutsidePathsAndPrefixLookalikesPassThroughWithoutBoardAccess)
{
    for (const char *path : {"/storage-other/file", "/dev/console", "temporary", "/stor"}) {
        FILE *file = __wrap_fopen(path, "rb");
        ASSERT_NE(file, nullptr);
        EXPECT_EQ(__wrap_fclose(file), 0);
    }
    EXPECT_EQ(begins.load(), 0u);
    EXPECT_EQ(ends.load(), 0u);
}
TEST_F(ReadPicoFs, MissingOrInvalidMediaRefusesBeforeAnyFilesystemOpen)
{
    for (auto error : {ESP_ERR_INVALID_STATE, ESP_ERR_NOT_FOUND, ESP_ERR_TIMEOUT}) {
        begin_result = error;
        EXPECT_EQ(__wrap_fopen("/storage/photo.jpg", "rb"), nullptr);
        EXPECT_EQ(__wrap_open("/storage/photo.jpg", O_RDONLY), -1);
        EXPECT_EQ(__wrap_opendir("/storage/images"), nullptr);
    }
    EXPECT_EQ(real_opens.load(), 0u);
    EXPECT_EQ(users.load(), 0u);
}
TEST_F(ReadPicoFs, OpenFailuresReleaseReservationsAndPreserveErrno)
{
    open_fail = true;
    EXPECT_EQ(__wrap_fopen("/storage/photo.jpg", "rb"), nullptr);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_EQ(__wrap_open("/storage/photo.jpg", O_RDONLY), -1);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_EQ(__wrap_opendir("/storage/images"), nullptr);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_EQ(begins.load(), 3u);
    EXPECT_EQ(ends.load(), 3u);
}
TEST_F(ReadPicoFs, FileCloseFailureRetainsLeaseAndNeverRetriesFreedHandle)
{
    FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
    ASSERT_NE(file, nullptr);
    close_fail = true;
    EXPECT_EQ(__wrap_fclose(file), EOF);
    EXPECT_TRUE(poisoned);
    EXPECT_EQ(users.load(), 1u);
    EXPECT_EQ(__wrap_fclose(file), EOF);
    EXPECT_EQ(real_closes.load(), 1u);
    EXPECT_EQ(__wrap_fopen("/storage/next.jpg", "rb"), nullptr);
}
TEST_F(ReadPicoFs, DescriptorToFileTransferDoesNotDoubleCountOrLoseLease)
{
    int fd = __wrap_open("/storage/new.png", O_WRONLY | O_CREAT | O_EXCL, 0600);
    ASSERT_GE(fd, 0);
    EXPECT_EQ(last_flags, O_WRONLY | O_CREAT | O_EXCL);
    EXPECT_EQ(last_mode, 0600);
    FILE *file = __wrap_fdopen(fd, "wb");
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(users.load(), 1u);
    EXPECT_EQ(__wrap_fclose(file), 0);
    EXPECT_EQ(ends.load(), 1u);
}
TEST_F(ReadPicoFs, FailedFdopenLeavesDescriptorLeaseForCallerCleanup)
{
    int fd = __wrap_open("/storage/new.png", O_RDONLY);
    ASSERT_GE(fd, 0);
    fdopen_fail = true;
    EXPECT_EQ(__wrap_fdopen(fd, "rb"), nullptr);
    EXPECT_EQ(users.load(), 1u);
    EXPECT_EQ(__wrap_close(fd), 0);
}
TEST_F(ReadPicoFs, DescriptorCloseFailureLatchesAndCannotBeRetried)
{
    int fd = __wrap_open("/storage/new.png", O_RDONLY);
    ASSERT_GE(fd, 0);
    close_fail = true;
    EXPECT_EQ(__wrap_close(fd), -1);
    EXPECT_EQ(__wrap_close(fd), -1);
    EXPECT_EQ(real_closes.load(), 1u);
    EXPECT_TRUE(poisoned);
    EXPECT_EQ(users.load(), 1u);
}
TEST_F(ReadPicoFs, DirectoryPinsMountUntilSuccessfulClose)
{
    DIR *dir = __wrap_opendir("/storage");
    ASSERT_NE(dir, nullptr);
    EXPECT_EQ(users.load(), 1u);
    observe_pending = true;
    EXPECT_EQ(__wrap_closedir(dir), 0);
    EXPECT_EQ(users.load(), 0u);
}
TEST_F(ReadPicoFs, DirectoryCloseFailureAlsoRetainsLeaseAndCannotRetry)
{
    DIR *dir = __wrap_opendir("/storage/images");
    ASSERT_NE(dir, nullptr);
    close_fail = true;
    EXPECT_EQ(__wrap_closedir(dir), -1);
    EXPECT_EQ(__wrap_closedir(dir), -1);
    EXPECT_EQ(real_closes.load(), 1u);
    EXPECT_TRUE(poisoned);
    EXPECT_EQ(users.load(), 1u);
}
TEST_F(ReadPicoFs, BoundedRegistryRefusesBeforeOpeningAndReusesReleasedSlots)
{
    std::vector<FILE *> files;
    for (int i = 0; i < 32; ++i) {
        FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
        ASSERT_NE(file, nullptr);
        files.push_back(file);
    }
    EXPECT_EQ(__wrap_fopen("/storage/overflow.jpg", "rb"), nullptr);
    EXPECT_EQ(errno, EMFILE);
    EXPECT_EQ(real_opens.load(), 32u);
    EXPECT_EQ(users.load(), 32u);
    for (FILE *file : files)
        EXPECT_EQ(__wrap_fclose(file), 0);
    FILE *again = __wrap_fopen("/storage/photo.jpg", "rb");
    ASSERT_NE(again, nullptr);
    EXPECT_EQ(__wrap_fclose(again), 0);
}
TEST_F(ReadPicoFs, MetadataOperationsPinOnlyStorageAndReleaseEvenOnFailure)
{
    struct stat info{};
    observe_pending = true;
    EXPECT_EQ(__wrap_stat("/storage", &info), 0);
    EXPECT_EQ(__wrap_mkdir("/storage/images", 0755), 0);
    EXPECT_EQ(__wrap_unlink("/storage/owned-temp"), 0);
    EXPECT_EQ(__wrap_rename("/outside/temp", "/storage/owned-temp"), 0);
    EXPECT_EQ(__wrap_rename("/storage/owned-temp", "/outside/temp"), 0);
    metadata_fail = true;
    EXPECT_EQ(__wrap_stat("/storage/absent", &info), -1);
    EXPECT_EQ(errno, ENOENT);
    EXPECT_EQ(begins.load(), 6u);
    EXPECT_EQ(ends.load(), 6u);
    observe_pending = false;
    EXPECT_EQ(__wrap_stat("/dev/console", &info), -1);
    EXPECT_EQ(begins.load(), 6u);
}
TEST_F(ReadPicoFs, ReusedHandleBeforeOldCloseBookkeepingPrefersNewLiveOwner)
{
    same_handle = reuse_during_close = true;
    FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(__wrap_fclose(file), 0);
    EXPECT_EQ(begins.load(), 2u);
    EXPECT_EQ(ends.load(), 2u);
    EXPECT_EQ(users.load(), 0u);
}

TEST_F(ReadPicoFs, ReusedHandleOutsideStorageDoesNotGetMistakenForClosingSdOwner)
{
    same_handle = reuse_during_close = true;
    reuse_path = "/dev/console";
    FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(__wrap_fclose(file), 0);
    EXPECT_EQ(begins.load(), 1u);
    EXPECT_EQ(ends.load(), 1u);
    EXPECT_EQ(real_closes.load(), 2u);
    EXPECT_EQ(users.load(), 0u);
}

TEST_F(ReadPicoFs, ConcurrentIndependentConsumersLeaveNoLostOrPhantomLeases)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
        threads.emplace_back([] {
            for (int iteration = 0; iteration < 50; ++iteration) {
                FILE *file = __wrap_fopen("/storage/photo.jpg", "rb");
                ASSERT_NE(file, nullptr);
                EXPECT_EQ(__wrap_fclose(file), 0);
            }
        });
    for (auto &thread : threads)
        thread.join();
    EXPECT_EQ(begins.load(), 400u);
    EXPECT_EQ(ends.load(), 400u);
    EXPECT_EQ(users.load(), 0u);
}
