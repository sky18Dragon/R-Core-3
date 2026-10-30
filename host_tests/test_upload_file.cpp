#include <gtest/gtest.h>

#include <string>

#include "upload_file.h"

namespace
{
struct File {
    int guards = 0, fail_guard = 0, creates = 0, writes = 0, syncs = 0, closes = 0, uncertain = 0,
        publishes = 0, removes = 0;
    bool fail_create = false, fail_sync = false, fail_close = false, fail_publish = false,
         fail_remove = false;
    std::string removed;
    int short_write = -1;
    std::string events, data;
    static File &get(void *p)
    {
        return *static_cast<File *>(p);
    }
    static bool ready(void *p)
    {
        auto &s = get(p);
        return ++s.guards != s.fail_guard;
    }
    static int create(void *p, const char *)
    {
        auto &s = get(p);
        ++s.creates;
        s.events += 'O';
        return s.fail_create ? -1 : 0;
    }
    static int write(void *p, int fd, const uint8_t *b, size_t n)
    {
        auto &s = get(p);
        EXPECT_EQ(fd, 0);
        ++s.writes;
        s.events += 'W';
        if (s.short_write >= 0)
            return s.short_write;
        s.data.append(reinterpret_cast<const char *>(b), n);
        return static_cast<int>(n);
    }
    static int sync(void *p, int fd)
    {
        auto &s = get(p);
        EXPECT_EQ(fd, 0);
        ++s.syncs;
        s.events += 'S';
        return s.fail_sync ? -1 : 0;
    }
    static int close(void *p, int fd)
    {
        auto &s = get(p);
        EXPECT_EQ(fd, 0);
        ++s.closes;
        s.events += 'C';
        return s.fail_close ? -1 : 0;
    }
    static void fail(void *p)
    {
        ++get(p).uncertain;
    }
    static int publish(void *p, const char *, const char *)
    {
        auto &s = get(p);
        ++s.publishes;
        s.events += 'P';
        return s.fail_publish ? -1 : 0;
    }
    static int remove(void *p, const char *path)
    {
        auto &s = get(p);
        ++s.removes;
        s.events += 'R';
        s.removed = path;
        return s.fail_remove ? -1 : 0;
    }
    upload_file_io_t io()
    {
        return {this, ready, create, write, sync, close, fail, publish, remove};
    }
};
bool begin(File &s, upload_file_t &f)
{
    auto io = s.io();
    return upload_file_begin(&f, &io, "hidden.part", 8);
}
bool write(upload_file_t &f)
{
    return upload_file_write(&f, reinterpret_cast<const uint8_t *>("data"), 4);
}
}  // namespace
TEST(UploadFile, SyncCloseBeforePublish)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    ASSERT_TRUE(upload_file_finish(&f, 4));
    ASSERT_TRUE(upload_file_publish(&f, "hidden.part", "new.png"));
    EXPECT_EQ(s.events, "OWSCP");
    EXPECT_EQ(s.data, "data");
    EXPECT_EQ(f.fd, -1);
    EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
    EXPECT_EQ(s.publishes, 1);
}
TEST(UploadFile, ShortAndZeroWritesNeverPublish)
{
    for (int n : {0, 2, 5}) {
        File s;
        s.short_write = n;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        EXPECT_FALSE(write(f));
        EXPECT_FALSE(upload_file_finish(&f, 4));
        EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
        EXPECT_EQ(s.closes, 1);
        EXPECT_EQ(s.syncs, 0);
        EXPECT_EQ(s.publishes, 0);
        upload_file_abort(&f);
        EXPECT_EQ(s.closes, 1);
    }
}
TEST(UploadFile, SyncFailureClosesOnceAndNeverPublishes)
{
    File s;
    s.fail_sync = true;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    EXPECT_FALSE(upload_file_finish(&f, 4));
    upload_file_abort(&f);
    EXPECT_EQ(s.closes, 1);
    EXPECT_EQ(s.uncertain, 0);
    EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
}
TEST(UploadFile, CloseFailureLatchesAndNoRetry)
{
    File s;
    s.fail_close = true;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    EXPECT_FALSE(upload_file_finish(&f, 4));
    EXPECT_TRUE(f.close_failed);
    EXPECT_EQ(s.uncertain, 1);
    upload_file_abort(&f);
    EXPECT_EQ(s.closes, 1);
    EXPECT_FALSE(upload_file_finish(&f, 4));
    EXPECT_EQ(s.publishes, 0);
}
TEST(UploadFile, IncompleteAndEmptyCloseWithoutSync)
{
    for (bool empty : {false, true}) {
        File s;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        if (!empty)
            ASSERT_TRUE(write(f));
        EXPECT_FALSE(upload_file_finish(&f, 5));
        EXPECT_EQ(s.closes, 1);
        EXPECT_EQ(s.syncs, 0);
    }
}
TEST(UploadFile, MissingMediaBeforeCreate)
{
    File s;
    s.fail_guard = 1;
    upload_file_t f;
    EXPECT_FALSE(begin(s, f));
    upload_file_abort(&f);
    EXPECT_EQ(s.creates, 0);
    EXPECT_EQ(s.closes, 0);
}
TEST(UploadFile, MediaLossBeforeOrAfterWrite)
{
    for (int guard : {2, 3}) {
        File s;
        s.fail_guard = guard;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        EXPECT_FALSE(write(f));
        upload_file_abort(&f);
        EXPECT_EQ(s.closes, 1);
        EXPECT_EQ(s.publishes, 0);
    }
}
TEST(UploadFile, MediaLossBeforeSyncOrAfterClose)
{
    for (int guard : {4, 5}) {
        File s;
        s.fail_guard = guard;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        ASSERT_TRUE(write(f));
        EXPECT_FALSE(upload_file_finish(&f, 4));
        EXPECT_EQ(s.closes, 1);
        EXPECT_FALSE(f.ready);
    }
}
TEST(UploadFile, MediaLossBeforeOrAfterRenameNoRollback)
{
    for (int guard : {6, 7}) {
        File s;
        s.fail_guard = guard;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        ASSERT_TRUE(write(f));
        ASSERT_TRUE(upload_file_finish(&f, 4));
        EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
        EXPECT_EQ(s.publishes, guard == 6 ? 0 : 1);
        EXPECT_EQ(f.published, guard == 7);
        EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
    }
}
TEST(UploadFile, ConflictRetainsReadySourceAndNoRetry)
{
    File s;
    s.fail_publish = true;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    ASSERT_TRUE(upload_file_finish(&f, 0));
    EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "existing.png"));
    EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "existing.png"));
    EXPECT_EQ(s.publishes, 1);
    EXPECT_FALSE(f.published);
    EXPECT_EQ(s.data, "data");
}
TEST(UploadFile, AbortReceiveFailureNoPublish)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    upload_file_abort(&f);
    upload_file_abort(&f);
    EXPECT_EQ(s.closes, 1);
    EXPECT_EQ(s.syncs, 0);
    EXPECT_FALSE(upload_file_finish(&f, 0));
    EXPECT_FALSE(upload_file_publish(&f, "x", "y"));
}
TEST(UploadFile, LimitAndInvalidInputNoWrite)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    EXPECT_FALSE(upload_file_write(&f, reinterpret_cast<const uint8_t *>("too large"), 9));
    EXPECT_EQ(s.writes, 0);
    upload_file_abort(&f);
    ASSERT_TRUE(begin(s, f));
    EXPECT_FALSE(upload_file_write(&f, nullptr, 1));
    upload_file_abort(&f);
}
TEST(UploadFile, CreateFailureDoesNotCloseUnknownHandle)
{
    File s;
    s.fail_create = true;
    upload_file_t f;
    EXPECT_FALSE(begin(s, f));
    upload_file_abort(&f);
    EXPECT_EQ(s.closes, 0);
    EXPECT_EQ(s.publishes, 0);
}
TEST(UploadFile, OtherSourceCannotBePublishedByOwnedTransaction)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    ASSERT_TRUE(upload_file_finish(&f, 4));
    EXPECT_FALSE(upload_file_publish(&f, "camera-original.jpg", "new.png"));
    EXPECT_EQ(s.publishes, 0);
    EXPECT_TRUE(upload_file_publish(&f, "hidden.part", "new.png"));
}
TEST(UploadFile, OverlongSourceRejectedBeforeCreate)
{
    File s;
    upload_file_t f = {};
    f.fd = -1;
    auto io = s.io();
    EXPECT_FALSE(upload_file_begin(&f, &io, std::string(512, 'x').c_str(), 8));
    EXPECT_EQ(s.creates, 0);
    upload_file_abort(&f);
    EXPECT_EQ(s.closes, 0);
}

TEST(UploadFile, DiscardRemovesOwnUnpublishedTemporaryOnce)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    ASSERT_TRUE(upload_file_finish(&f, 4));
    EXPECT_TRUE(upload_file_discard(&f));
    EXPECT_EQ(s.events, "OWSCR");
    EXPECT_EQ(s.removed, "hidden.part");
    EXPECT_FALSE(upload_file_discard(&f));  // already gone; never a second remove
    EXPECT_FALSE(upload_file_publish(&f, "hidden.part", "new.png"));
    EXPECT_EQ(s.removes, 1);
}
TEST(UploadFile, DiscardClosesAnOpenFailedUploadFirst)
{
    File s;
    upload_file_t f;
    ASSERT_TRUE(begin(s, f));
    ASSERT_TRUE(write(f));
    EXPECT_TRUE(upload_file_discard(&f));
    EXPECT_EQ(s.events, "OWCR");
    EXPECT_EQ(s.closes, 1);
}
TEST(UploadFile, DiscardNeverRemovesWhatItDidNotCreateOrCannotTrust)
{
    {   // exclusive create failed: the path may belong to someone else
        File s;
        s.fail_create = true;
        upload_file_t f;
        EXPECT_FALSE(begin(s, f));
        EXPECT_FALSE(upload_file_discard(&f));
        EXPECT_EQ(s.removes, 0);
    }
    {   // published: the file is now an album original
        File s;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        ASSERT_TRUE(write(f));
        ASSERT_TRUE(upload_file_finish(&f, 4));
        ASSERT_TRUE(upload_file_publish(&f, "hidden.part", "new.png"));
        EXPECT_FALSE(upload_file_discard(&f));
        EXPECT_EQ(s.removes, 0);
    }
    {   // close failed: the handle state is unknown
        File s;
        s.fail_close = true;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        ASSERT_TRUE(write(f));
        EXPECT_FALSE(upload_file_discard(&f));
        EXPECT_EQ(s.removes, 0);
        EXPECT_EQ(s.uncertain, 1);
    }
    {   // adapter without a remover keeps the file
        File s;
        upload_file_t f;
        auto io = s.io();
        io.remove_temp = nullptr;
        ASSERT_TRUE(upload_file_begin(&f, &io, "hidden.part", 8));
        EXPECT_FALSE(upload_file_discard(&f));
    }
    {   // remover failure is reported, not hidden
        File s;
        s.fail_remove = true;
        upload_file_t f;
        ASSERT_TRUE(begin(s, f));
        EXPECT_FALSE(upload_file_discard(&f));
        EXPECT_EQ(s.removes, 1);
    }
}
