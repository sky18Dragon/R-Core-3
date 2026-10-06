#include <gtest/gtest.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "sd_file_probe.h"

namespace {
struct Fake {
    std::vector<uint8_t> file;
    std::string events;
    int opens = 0, closes = 0, guards = 0;
    int fail_guard = 0, fail_close = 0;
    int write_result = 256, read_result = 256;
    bool create_fail = false, reopen_fail = false, sync_fail = false;
    bool corrupt = false, truncate = false, append = false;
    bool eof_fail = false;
    size_t cursor = 0;
    static Fake &get(void *ctx) { return *static_cast<Fake *>(ctx); }
    static bool ready(void *ctx) {
        auto &f = get(ctx);
        return ++f.guards != f.fail_guard;
    }
    static int create(void *ctx) {
        auto &f = get(ctx);
        f.events += "N";
        ++f.opens;
        return f.create_fail ? -1 : 0;  // Descriptor zero is valid.
    }
    static int reopen(void *ctx) {
        auto &f = get(ctx);
        f.events += "R";
        ++f.opens;
        f.cursor = 0;
        if (f.corrupt) f.file[1234] ^= 1;
        if (f.truncate) f.file.resize(2000);
        if (f.append) f.file.push_back(0);
        return f.reopen_fail ? -1 : 1;
    }
    static int write(void *ctx, int fd, const uint8_t *data, size_t size) {
        auto &f = get(ctx);
        EXPECT_EQ(fd, 0);
        f.events += "W";
        if (f.write_result <= 0 || static_cast<size_t>(f.write_result) > SD_FILE_PROBE_CHUNK)
            return f.write_result;
        size_t count = std::min(size, static_cast<size_t>(f.write_result));
        f.file.insert(f.file.end(), data, data + count);
        return static_cast<int>(count);
    }
    static int read(void *ctx, int fd, uint8_t *data, size_t size) {
        auto &f = get(ctx);
        EXPECT_EQ(fd, 1);
        f.events += "D";
        if (f.read_result < 0 || static_cast<size_t>(f.read_result) > SD_FILE_PROBE_CHUNK)
            return f.read_result;
        if (f.eof_fail && f.cursor == f.file.size())
            return -1;
        size_t count = std::min({size, f.file.size() - f.cursor,
                                 static_cast<size_t>(f.read_result)});
        std::copy_n(f.file.begin() + f.cursor, count, data);
        f.cursor += count;
        return static_cast<int>(count);
    }
    static int sync(void *ctx, int fd) {
        auto &f = get(ctx);
        EXPECT_EQ(fd, 0);
        f.events += "S";
        return f.sync_fail ? -1 : 0;
    }
    static int close(void *ctx, int fd) {
        auto &f = get(ctx);
        EXPECT_EQ(fd, f.events.find('N') != std::string::npos && f.closes == 0 ? 0 : 1);
        f.events += "C";
        return ++f.closes == f.fail_close ? -1 : 0;
    }
    sd_file_probe_io_t io() {
        return {this, ready, create, reopen, write, read, sync, close};
    }
    sd_file_probe_read_io_t read_io() {
        return {this, ready, reopen, read, close};
    }
};
}

class SdFileProbeTest : public testing::Test {
protected:
    Fake fake;
    sd_file_probe_session_t session{};
    sd_file_probe_result_t run() {
        auto io = fake.io();
        return sd_file_probe_run(&session, &io);
    }
};

TEST_F(SdFileProbeTest, WritesSyncsClosesReopensVerifiesAllBytesAndEof) {
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_OK);
    EXPECT_TRUE(result.created && session.attempted);
    EXPECT_FALSE(session.close_failed);
    EXPECT_EQ(result.written, 4096u);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(fake.events, "N" + std::string(16, 'W') + "SCR" + std::string(17, 'D') + "C");
    ASSERT_EQ(fake.file.size(), 4096u);
    for (size_t i = 0; i < fake.file.size(); ++i)
        EXPECT_EQ(fake.file[i], static_cast<uint8_t>(i * 37u + (i / 256u) * 17u + 0x5au));
}

TEST_F(SdFileProbeTest, PartialTransfersRemainBoundedAndCorrect) {
    fake.write_result = 37;
    fake.read_result = 19;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_OK);
    EXPECT_EQ(result.written, 4096u);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(fake.closes, 2);
}

TEST_F(SdFileProbeTest, NoMediaDoesNotOpenAndConsumesSingleAttempt) {
    fake.fail_guard = 1;
    EXPECT_EQ(run().error, SD_FILE_PROBE_MEDIA_UNAVAILABLE);
    EXPECT_EQ(fake.opens, 0);
    EXPECT_EQ(run().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.opens, 0);
}

TEST_F(SdFileProbeTest, ExistingOrFailedCreateNeverReadsWritesOrCloses) {
    fake.file = {1, 2, 3};
    fake.create_fail = true;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_CREATE_FAILED);
    EXPECT_FALSE(result.created);
    EXPECT_EQ(fake.file, (std::vector<uint8_t>{1, 2, 3}));
    EXPECT_EQ(fake.events, "N");
    EXPECT_EQ(run().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
}

TEST_F(SdFileProbeTest, SecondSuccessfulRequestCannotReopenOrOverwrite) {
    EXPECT_EQ(run().error, SD_FILE_PROBE_OK);
    auto events = fake.events;
    EXPECT_EQ(run().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.events, events);
}

TEST_F(SdFileProbeTest, ZeroNegativeAndOversizeWriteFailWithoutRetry) {
    for (int count : {0, -1, 257}) {
        fake = Fake{};
        session = {};
        fake.write_result = count;
        auto result = run();
        EXPECT_EQ(result.error, SD_FILE_PROBE_WRITE_FAILED);
        EXPECT_TRUE(result.created);
        EXPECT_EQ(result.written, 0u);
        EXPECT_EQ(fake.events, "NWC");
    }
}

TEST_F(SdFileProbeTest, MediaLossDuringWriteClosesWithoutSyncOrReopen) {
    fake.fail_guard = 3;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_MEDIA_UNAVAILABLE);
    EXPECT_EQ(result.written, 256u);
    EXPECT_EQ(fake.events, "NWC");
}

TEST_F(SdFileProbeTest, SyncFailureClosesAndDoesNotReopenOrDelete) {
    fake.sync_fail = true;
    EXPECT_EQ(run().error, SD_FILE_PROBE_SYNC_FAILED);
    EXPECT_EQ(fake.closes, 1);
    EXPECT_EQ(fake.opens, 1);
    EXPECT_EQ(fake.file.size(), 4096u);
}

TEST_F(SdFileProbeTest, WriteCloseFailureLatchesAndDoesNotRetryOrReopen) {
    fake.fail_close = 1;
    EXPECT_EQ(run().error, SD_FILE_PROBE_CLOSE_FAILED);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(fake.closes, 1);
    EXPECT_EQ(fake.opens, 1);
    EXPECT_EQ(run().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.closes, 1);
}

TEST_F(SdFileProbeTest, PrimaryAndCloseErrorsAreNotHidden) {
    fake.write_result = -1;
    fake.fail_close = 1;
    EXPECT_EQ(run().error, SD_FILE_PROBE_WRITE_FAILED);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(fake.closes, 1);
}

TEST_F(SdFileProbeTest, ReopenFailureKeepsFileAndHasNoSecondClose) {
    fake.reopen_fail = true;
    EXPECT_EQ(run().error, SD_FILE_PROBE_REOPEN_FAILED);
    EXPECT_EQ(fake.closes, 1);
    EXPECT_EQ(fake.opens, 2);
    EXPECT_EQ(fake.file.size(), 4096u);
}

TEST_F(SdFileProbeTest, CorruptByteReportsExactVerifiedPrefix) {
    fake.corrupt = true;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_MISMATCH);
    EXPECT_EQ(result.verified, 1234u);
    EXPECT_EQ(fake.closes, 2);
}

TEST_F(SdFileProbeTest, ShortFileDoesNotPass) {
    fake.truncate = true;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_READ_FAILED);
    EXPECT_EQ(result.verified, 2000u);
    EXPECT_EQ(fake.closes, 2);
}

TEST_F(SdFileProbeTest, ExtraTrailingByteDoesNotPass) {
    fake.append = true;
    auto result = run();
    EXPECT_EQ(result.error, SD_FILE_PROBE_EXTRA_DATA);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(fake.closes, 2);
}

TEST_F(SdFileProbeTest, NegativeAndOversizeReadFailAndClose) {
    for (int count : {-1, 257}) {
        fake = Fake{};
        session = {};
        fake.read_result = count;
        EXPECT_EQ(run().error, SD_FILE_PROBE_READ_FAILED);
        EXPECT_EQ(fake.closes, 2);
    }
}

TEST_F(SdFileProbeTest, ReadCloseFailurePreventsSuccessAndLatches) {
    fake.fail_close = 2;
    EXPECT_EQ(run().error, SD_FILE_PROBE_CLOSE_FAILED);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(fake.closes, 2);
}

TEST_F(SdFileProbeTest, EveryMediaFailurePointClosesAllOpenedHandles) {
    // Baseline checks: initial, 16 writes, sync, reopen, 16 reads, EOF, final.
    for (int guard = 1; guard <= 37; ++guard) {
        fake = Fake{};
        session = {};
        fake.fail_guard = guard;
        auto result = run();
        EXPECT_EQ(result.error, SD_FILE_PROBE_MEDIA_UNAVAILABLE) << guard;
        EXPECT_EQ(fake.closes, fake.opens) << guard;
        EXPECT_FALSE(session.close_failed);
    }
}

TEST_F(SdFileProbeTest, InvalidCallbacksNeverConsumeAttemptOrTouchIo) {
    auto io = fake.io();
    io.close = nullptr;
    EXPECT_EQ(sd_file_probe_run(&session, &io).error, SD_FILE_PROBE_INVALID);
    EXPECT_EQ(sd_file_probe_run(nullptr, &io).error, SD_FILE_PROBE_INVALID);
    EXPECT_EQ(sd_file_probe_run(&session, nullptr).error, SD_FILE_PROBE_INVALID);
    EXPECT_FALSE(session.attempted);
    EXPECT_TRUE(fake.events.empty());
    EXPECT_STREQ(sd_file_probe_error_name(SD_FILE_PROBE_OK), "OK");
}

class SdFileCheckTest : public SdFileProbeTest {
protected:
    void fill() {
        fake.file.resize(4096);
        for (size_t i = 0; i < fake.file.size(); ++i)
            fake.file[i] = static_cast<uint8_t>(i * 37u + (i / 256u) * 17u + 0x5au);
    }
    void SetUp() override { fill(); }
    sd_file_probe_result_t check() {
        auto io = fake.read_io();
        return sd_file_probe_check(&session, &io);
    }
};

TEST_F(SdFileCheckTest, ExistingFileOnlyReadsExactBytesAndEofWithoutMutation) {
    auto before = fake.file;
    auto result = check();
    EXPECT_EQ(result.error, SD_FILE_PROBE_OK);
    EXPECT_FALSE(result.created || session.attempted || session.close_failed);
    EXPECT_TRUE(session.check_attempted);
    EXPECT_EQ(result.written, 0u);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(fake.events, "R" + std::string(17, 'D') + "C");
    EXPECT_EQ(fake.file, before);
}

TEST_F(SdFileCheckTest, MissingFileFailsWithoutCreateOrRepair) {
    fake.file.clear();
    fake.reopen_fail = true;
    auto result = check();
    EXPECT_EQ(result.error, SD_FILE_PROBE_READ_OPEN_FAILED);
    EXPECT_FALSE(result.created);
    EXPECT_TRUE(fake.file.empty());
    EXPECT_EQ(fake.events, "R");
    EXPECT_EQ(fake.closes, 0);
    EXPECT_EQ(check().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
}

TEST_F(SdFileCheckTest, NoMediaDoesNotOpenOrConsumeWriteAttempt) {
    fake.fail_guard = 1;
    EXPECT_EQ(check().error, SD_FILE_PROBE_MEDIA_UNAVAILABLE);
    EXPECT_EQ(fake.opens, 0);
    EXPECT_TRUE(session.check_attempted);
    EXPECT_FALSE(session.attempted);
    EXPECT_EQ(check().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.opens, 0);
}

TEST_F(SdFileCheckTest, RepeatRequestDoesNotRepeatFileIo) {
    EXPECT_EQ(check().error, SD_FILE_PROBE_OK);
    auto before = fake.events;
    EXPECT_EQ(check().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.events, before);
}

TEST_F(SdFileCheckTest, CorruptionIsReportedAndNeverRepaired) {
    fake.file[1234] ^= 1;
    auto before = fake.file;
    auto result = check();
    EXPECT_EQ(result.error, SD_FILE_PROBE_MISMATCH);
    EXPECT_EQ(result.verified, 1234u);
    EXPECT_EQ(fake.file, before);
    EXPECT_EQ(fake.closes, 1);
}

TEST_F(SdFileCheckTest, ShortAndExtraFilesAreNotAcceptedOrChanged) {
    for (size_t size : {2000u, 4097u}) {
        fake = Fake{};
        session = {};
        fill();
        fake.file.resize(size);
        auto before = fake.file;
        auto result = check();
        EXPECT_EQ(result.error, size == 2000 ? SD_FILE_PROBE_READ_FAILED : SD_FILE_PROBE_EXTRA_DATA);
        EXPECT_EQ(fake.file, before);
        EXPECT_EQ(fake.closes, 1);
    }
}

TEST_F(SdFileCheckTest, PartialReadsAndZeroNegativeOversizeResultsAreHandled) {
    fake.read_result = 19;
    EXPECT_EQ(check().error, SD_FILE_PROBE_OK);
    for (int count : {0, -1, 257}) {
        fake = Fake{};
        session = {};
        fill();
        fake.read_result = count;
        EXPECT_EQ(check().error, SD_FILE_PROBE_READ_FAILED);
        EXPECT_EQ(fake.closes, 1);
    }
}

TEST_F(SdFileCheckTest, EveryReadMediaLossPointClosesAllOpenedHandles) {
    // Initial, 16 reads, EOF, final: all 19 boundaries.
    for (int guard = 1; guard <= 19; ++guard) {
        fake = Fake{};
        session = {};
        fill();
        fake.fail_guard = guard;
        auto result = check();
        EXPECT_EQ(result.error, SD_FILE_PROBE_MEDIA_UNAVAILABLE) << guard;
        EXPECT_EQ(fake.closes, fake.opens) << guard;
        EXPECT_EQ(fake.events.find('N'), std::string::npos);
        EXPECT_EQ(fake.events.find('W'), std::string::npos);
        EXPECT_EQ(fake.events.find('S'), std::string::npos);
    }
}

TEST_F(SdFileCheckTest, ErrorAtEofDoesNotPassEvenAfterAllBytesMatch) {
    fake.eof_fail = true;
    auto result = check();
    EXPECT_EQ(result.error, SD_FILE_PROBE_READ_FAILED);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(fake.closes, 1);
    EXPECT_FALSE(session.close_failed);
}

TEST_F(SdFileCheckTest, CloseFailureLatchesAndBlocksBothSubsequentOperations) {
    fake.fail_close = 1;
    EXPECT_EQ(check().error, SD_FILE_PROBE_CLOSE_FAILED);
    EXPECT_TRUE(session.close_failed);
    auto before = fake.events;
    EXPECT_EQ(run().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(check().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.events, before);
}

TEST_F(SdFileCheckTest, PrimaryReadFailureAndCloseFailureRemainSeparate) {
    fake.fail_close = 1;
    fake.read_result = -1;
    EXPECT_EQ(check().error, SD_FILE_PROBE_READ_FAILED);
    EXPECT_TRUE(session.close_failed);
    EXPECT_EQ(fake.closes, 1);
}

TEST_F(SdFileCheckTest, WriterCloseFailureAlsoBlocksIndependentReadCheck) {
    fake.file.clear();
    fake.fail_close = 1;
    EXPECT_EQ(run().error, SD_FILE_PROBE_CLOSE_FAILED);
    auto before = fake.events;
    EXPECT_EQ(check().error, SD_FILE_PROBE_ALREADY_ATTEMPTED);
    EXPECT_EQ(fake.events, before);
}

TEST_F(SdFileCheckTest, InvalidReadCallbacksDoNotConsumeAnyAttempt) {
    auto io = fake.read_io();
    io.close = nullptr;
    EXPECT_EQ(sd_file_probe_check(&session, &io).error, SD_FILE_PROBE_INVALID);
    EXPECT_EQ(sd_file_probe_check(nullptr, &io).error, SD_FILE_PROBE_INVALID);
    EXPECT_EQ(sd_file_probe_check(&session, nullptr).error, SD_FILE_PROBE_INVALID);
    EXPECT_FALSE(session.attempted || session.check_attempted || session.close_failed);
    EXPECT_TRUE(fake.events.empty());
}

// Actual host POSIX path semantics, using only our temporary directory/file.
TEST(SdFileProbePosixTest, ExclusiveCreateSyncReopenAndConflictPreserveExistingBytes) {
    char directory[] = "/tmp/rc3-sd-probe-XXXXXX";
    ASSERT_NE(mkdtemp(directory), nullptr);
    std::string path = std::string(directory) + "/RC3T0001.BIN";
    sd_file_probe_io_t io = {
        &path,
        [](void *) { return true; },
        [](void *ctx) { return open(static_cast<std::string *>(ctx)->c_str(),
                                   O_WRONLY | O_CREAT | O_EXCL, 0600); },
        [](void *ctx) { return open(static_cast<std::string *>(ctx)->c_str(), O_RDONLY); },
        [](void *, int fd, const uint8_t *data, size_t size) { return static_cast<int>(write(fd, data, size)); },
        [](void *, int fd, uint8_t *data, size_t size) { return static_cast<int>(read(fd, data, size)); },
        [](void *, int fd) { return fsync(fd); },
        [](void *, int fd) { return close(fd); },
    };
    sd_file_probe_session_t session{};
    auto result = sd_file_probe_run(&session, &io);
    EXPECT_EQ(result.error, SD_FILE_PROBE_OK);
    session = {};
    result = sd_file_probe_run(&session, &io);
    EXPECT_EQ(result.error, SD_FILE_PROBE_CREATE_FAILED);
    EXPECT_EQ(errno, EEXIST);
    EXPECT_FALSE(result.created);
    int fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    std::vector<uint8_t> bytes(4097);
    EXPECT_EQ(read(fd, bytes.data(), bytes.size()), 4096);
    EXPECT_EQ(close(fd), 0);
    for (size_t i = 0; i < 4096; ++i)
        EXPECT_EQ(bytes[i], sd_file_probe_byte(i));
    // A fresh session can check the retained file with only read capabilities.
    sd_file_probe_read_io_t read_io = {io.context, io.media_ready, io.open_read, io.read, io.close};
    session = {};
    result = sd_file_probe_check(&session, &read_io);
    EXPECT_EQ(result.error, SD_FILE_PROBE_OK);
    EXPECT_EQ(result.verified, 4096u);
    EXPECT_EQ(result.written, 0u);
    EXPECT_FALSE(result.created);
    // Only this host test's new file is removed, never a card/private path.
    EXPECT_EQ(unlink(path.c_str()), 0);
    EXPECT_EQ(rmdir(directory), 0);
}
