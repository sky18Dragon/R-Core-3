#include <gtest/gtest.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

extern "C" {
#include "upload_sweep.h"
}

namespace
{
class UploadSweep : public testing::Test
{
   protected:
    std::string root;
    void SetUp() override
    {
        char tmpl[] = "/tmp/upload_sweep_XXXXXX";
        ASSERT_NE(mkdtemp(tmpl), nullptr);
        root = tmpl;
    }
    void TearDown() override { std::system(("rm -rf '" + root + "'").c_str()); }
    void mkdir_(const std::string &rel) { ASSERT_EQ(mkdir((root + "/" + rel).c_str(), 0700), 0); }
    void file(const std::string &rel, size_t size = 10)
    {
        std::ofstream(root + "/" + rel) << std::string(size, 'x');
    }
    bool exists(const std::string &rel)
    {
        struct stat st;
        return stat((root + "/" + rel).c_str(), &st) == 0;
    }
};
const char *kTemp = ".rc3-upload-12345678-0000abcd.part";
}  // namespace

TEST(UploadSweepName, MatchesOnlyExactUploadTemporaryNames)
{
    EXPECT_TRUE(upload_sweep_is_temp_name(kTemp));
    EXPECT_FALSE(upload_sweep_is_temp_name(".rc3-upload-12345678-0000ABCD.part"));
    EXPECT_FALSE(upload_sweep_is_temp_name(".rc3-upload-1234567-0000abcd.part"));
    EXPECT_FALSE(upload_sweep_is_temp_name(".rc3-upload-12345678-0000abcd.part.jpg"));
    EXPECT_FALSE(upload_sweep_is_temp_name(".rc3-upload-12345678_0000abcd.part"));
    EXPECT_FALSE(upload_sweep_is_temp_name("photo.epdgz"));
    EXPECT_FALSE(upload_sweep_is_temp_name("rc3-upload-12345678-0000abcd.part"));
    EXPECT_FALSE(upload_sweep_is_temp_name(nullptr));
}

TEST_F(UploadSweep, RemovesOnlyTemporariesDirectlyInAlbums)
{
    mkdir_("Default");
    mkdir_("Trips");
    mkdir_("Trips/nested");
    file(std::string("Default/") + kTemp, 100);
    file("Default/.rc3-upload-aaaaaaaa-bbbbbbbb.part", 20);
    file("Default/photo.epdgz");
    file("Default/photo.jpg");
    file("Default/.rc3-upload-notes.txt");
    file(std::string("Trips/nested/") + kTemp);  // deeper than album level
    file(kTemp);                                 // root level: not an album
    mkdir_(std::string("Trips/") + ".rc3-upload-cccccccc-dddddddd.part");  // directory, not a file
    upload_sweep_result_t r;
    ASSERT_EQ(upload_sweep_orphans(root.c_str(), 64, &r), 0);
    EXPECT_EQ(r.removed, 2u);
    EXPECT_EQ(r.bytes, 120u);
    EXPECT_EQ(r.failed, 0u);
    EXPECT_FALSE(r.truncated);
    EXPECT_FALSE(exists(std::string("Default/") + kTemp));
    EXPECT_TRUE(exists("Default/photo.epdgz"));
    EXPECT_TRUE(exists("Default/photo.jpg"));
    EXPECT_TRUE(exists("Default/.rc3-upload-notes.txt"));
    EXPECT_TRUE(exists(std::string("Trips/nested/") + kTemp));
    EXPECT_TRUE(exists(kTemp));
    EXPECT_TRUE(exists("Trips/.rc3-upload-cccccccc-dddddddd.part"));
}

TEST_F(UploadSweep, StopsAtRemovalLimit)
{
    mkdir_("Default");
    for (int i = 0; i < 5; ++i) {
        char name[64];
        snprintf(name, sizeof(name), "Default/.rc3-upload-%08x-00000000.part", i);
        file(name);
    }
    upload_sweep_result_t r;
    ASSERT_EQ(upload_sweep_orphans(root.c_str(), 3, &r), 0);
    EXPECT_EQ(r.removed, 3u);
    EXPECT_TRUE(r.truncated);
    ASSERT_EQ(upload_sweep_orphans(root.c_str(), 3, &r), 0);
    EXPECT_EQ(r.removed, 2u);
    EXPECT_FALSE(r.truncated);
}

TEST_F(UploadSweep, MissingRootReportsFailureWithZeroResult)
{
    upload_sweep_result_t r;
    r.removed = 9;
    EXPECT_EQ(upload_sweep_orphans((root + "/missing").c_str(), 3, &r), -1);
    EXPECT_EQ(r.removed, 0u);
    EXPECT_EQ(upload_sweep_orphans(nullptr, 3, nullptr), -1);
}
