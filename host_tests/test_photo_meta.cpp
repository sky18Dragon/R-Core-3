#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

extern "C" {
#include "photo_meta.h"
}

namespace
{
struct Index {
    cJSON *json;
    explicit Index(const char *text = nullptr) : json(photo_meta_parse(text)) {}
    ~Index() { cJSON_Delete(json); }
};

std::string make_dir()
{
    char path[] = "/tmp/photo-meta-XXXXXX";
    EXPECT_NE(mkdtemp(path), nullptr);
    return path;
}

void write(const std::string &path, const std::string &text) { std::ofstream(path) << text; }
}  // namespace

TEST(PhotoMeta, UpValuesAndUploadMapping)
{
    for (int up : {0, 90, 180, 270}) EXPECT_TRUE(photo_meta_up_valid(up));
    for (int up : {-90, 45, 360, 1}) EXPECT_FALSE(photo_meta_up_valid(up));
    // A portrait source turned to fill the panel is undone by turning back.
    EXPECT_EQ(photo_meta_up_for_upload(90, true), 270);
    EXPECT_EQ(photo_meta_up_for_upload(-90, true), 90);
    EXPECT_EQ(photo_meta_up_for_upload(270, true), 90);
    // Any other turn corrected the photo: it is stored upright.
    EXPECT_EQ(photo_meta_up_for_upload(0, true), 0);
    EXPECT_EQ(photo_meta_up_for_upload(180, true), 0);
    EXPECT_EQ(photo_meta_up_for_upload(90, false), 0);
}

TEST(PhotoMeta, TurnForTheFramesOrientation)
{
    EXPECT_EQ(photo_meta_turn(0, 0), 0);
    EXPECT_EQ(photo_meta_turn(180, 0), 180);  // landscape photo stored upside down
    EXPECT_EQ(photo_meta_turn(90, 0), 90);    // portrait photo on a landscape frame
    EXPECT_EQ(photo_meta_turn(90, 90), 0);
    EXPECT_EQ(photo_meta_turn(90, 270), 180);
    EXPECT_EQ(photo_meta_turn(0, 270), 90);
}

TEST(PhotoMeta, UnknownAndBrokenIndexesMeanLandscape)
{
    for (const char *text : {(const char *) nullptr, "", "not json", "[1,2]", "{\"a.epdgz\":5}",
                             "{\"a.epdgz\":{\"up\":45,\"added\":-3}}"}) {
        Index index(text);
        ASSERT_TRUE(cJSON_IsObject(index.json)) << (text ? text : "null");
        EXPECT_EQ(photo_meta_up(index.json, "a.epdgz"), 0);
        EXPECT_EQ(photo_meta_added(index.json, "a.epdgz"), 0);
    }
    EXPECT_EQ(photo_meta_up(nullptr, "a"), 0);
}

TEST(PhotoMeta, SetKeepsTheFirstAddedTimeAndForgets)
{
    Index index;
    EXPECT_TRUE(photo_meta_set(index.json, "a.epdgz", 90, 1793700000));
    EXPECT_EQ(photo_meta_up(index.json, "a.epdgz"), 90);
    EXPECT_EQ(photo_meta_added(index.json, "a.epdgz"), 1793700000);
    EXPECT_TRUE(photo_meta_set(index.json, "a.epdgz", 180, 1800000000));
    EXPECT_EQ(photo_meta_up(index.json, "a.epdgz"), 180);
    EXPECT_EQ(photo_meta_added(index.json, "a.epdgz"), 1793700000);
    EXPECT_TRUE(photo_meta_set(index.json, "b.epdgz", 0, 0));  // clock unknown: no time
    EXPECT_EQ(photo_meta_added(index.json, "b.epdgz"), 0);
    EXPECT_FALSE(photo_meta_set(index.json, "c.epdgz", 45, 0));
    EXPECT_FALSE(photo_meta_set(index.json, "", 0, 0));
    EXPECT_FALSE(photo_meta_set(nullptr, "c", 0, 0));
    photo_meta_forget(index.json, "a.epdgz");
    EXPECT_EQ(photo_meta_up(index.json, "a.epdgz"), 0);
    EXPECT_EQ(photo_meta_up(index.json, "b.epdgz"), 0);
}

TEST(PhotoMeta, SaveLoadAndLookupInAnAlbumDirectory)
{
    std::string dir = make_dir();
    Index index;
    ASSERT_TRUE(photo_meta_set(index.json, "x 1.epdgz", 270, 1793700000));
    ASSERT_TRUE(photo_meta_save(dir.c_str(), index.json));
    EXPECT_TRUE(std::filesystem::exists(dir + "/" PHOTO_META_FILENAME));
    EXPECT_FALSE(std::filesystem::exists(dir + "/" PHOTO_META_FILENAME ".new"));
    EXPECT_FALSE(std::filesystem::exists(dir + "/" PHOTO_META_FILENAME ".old"));
    cJSON *loaded = photo_meta_load(dir.c_str());
    EXPECT_EQ(photo_meta_up(loaded, "x 1.epdgz"), 270);
    cJSON_Delete(loaded);
    EXPECT_EQ(photo_meta_lookup_up((dir + "/x 1.epdgz").c_str()), 270);
    EXPECT_EQ(photo_meta_lookup_up((dir + "/other.epdgz").c_str()), 0);
    EXPECT_EQ(photo_meta_lookup_up("no-slash"), 0);
    // A second save replaces the first and leaves no side files.
    ASSERT_TRUE(photo_meta_set(index.json, "x 1.epdgz", 0, 0));
    ASSERT_TRUE(photo_meta_save(dir.c_str(), index.json));
    EXPECT_EQ(photo_meta_lookup_up((dir + "/x 1.epdgz").c_str()), 0);
    EXPECT_FALSE(std::filesystem::exists(dir + "/" PHOTO_META_FILENAME ".old"));
    std::filesystem::remove_all(dir);
}

TEST(PhotoMeta, InterruptedSaveFallsBackToTheParkedCopy)
{
    std::string dir = make_dir();
    // Power cut after parking the old index and before the new one took its place.
    write(dir + "/" PHOTO_META_FILENAME ".old", "{\"a.epdgz\":{\"up\":90}}");
    write(dir + "/" PHOTO_META_FILENAME ".new", "{\"a.epdgz\":{\"up\":18");
    EXPECT_EQ(photo_meta_lookup_up((dir + "/a.epdgz").c_str()), 90);
    // An oversize or missing album directory reads as empty.
    write(dir + "/" PHOTO_META_FILENAME, std::string(PHOTO_META_MAX_BYTES + 1, ' '));
    EXPECT_EQ(photo_meta_lookup_up((dir + "/a.epdgz").c_str()), 90);
    EXPECT_EQ(photo_meta_lookup_up("/tmp/photo-meta-missing/a.epdgz"), 0);
    Index index;
    EXPECT_FALSE(photo_meta_save("/tmp/photo-meta-missing", index.json));
    std::filesystem::remove_all(dir);
}

TEST(PhotoMeta, HalfTurnOfAPackedFourBitFrame)
{
    // 4x2 pixels, high nibble first: row 0 = 1 2 3 4, row 1 = 5 6 7 8
    std::vector<uint8_t> frame = {0x12, 0x34, 0x56, 0x78};
    photo_meta_rotate180_4bpp(frame.data(), frame.size());
    EXPECT_EQ(frame, (std::vector<uint8_t>{0x87, 0x65, 0x43, 0x21}));
    photo_meta_rotate180_4bpp(frame.data(), frame.size());
    EXPECT_EQ(frame, (std::vector<uint8_t>{0x12, 0x34, 0x56, 0x78}));
    std::vector<uint8_t> odd = {0x12, 0x34, 0x56};
    photo_meta_rotate180_4bpp(odd.data(), odd.size());
    EXPECT_EQ(odd, (std::vector<uint8_t>{0x65, 0x43, 0x21}));
    photo_meta_rotate180_4bpp(nullptr, 4);
}
