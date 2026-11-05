#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

extern "C" {
#include "preset_store.h"
}

namespace
{
struct Doc {
    cJSON *json;
    explicit Doc(cJSON *j) : json(j) {}
    ~Doc() { cJSON_Delete(json); }
};

cJSON *parse(const std::string &text) { return cJSON_Parse(text.c_str()); }

std::string preset(const std::string &id, const std::string &name, const std::string &params)
{
    return "{\"id\":\"" + id + "\",\"name\":\"" + name + "\",\"params\":" + params + "}";
}

std::string doc(const std::string &presets, const std::string &def = "null", int rev = 0)
{
    return "{\"version\":1,\"rev\":" + std::to_string(rev) + ",\"default\":" + def +
           ",\"presets\":[" + presets + "]}";
}

const char *kParams =
    "{\"exposure\":0.97,\"contrast\":1.21,\"toneMode\":\"contrast\",\"colorMethod\":\"rgb\","
    "\"ditherAlgorithm\":\"stucki\",\"compressDynamicRange\":true}";

std::string make_dir()
{
    char path[] = "/tmp/preset-store-XXXXXX";
    EXPECT_NE(mkdtemp(path), nullptr);
    return path;
}
}  // namespace

TEST(PresetStore, EmptyDocument)
{
    Doc empty(preset_store_empty());
    EXPECT_EQ(preset_store_rev(empty.json), 0);
    EXPECT_TRUE(cJSON_IsNull(cJSON_GetObjectItem(empty.json, "default")));
    EXPECT_EQ(cJSON_GetArraySize(cJSON_GetObjectItem(empty.json, "presets")), 0);
}

TEST(PresetStore, KeepsKnownFieldsAndTheClientsRev)
{
    Doc sent(parse(doc(preset("p1", "人像", kParams).insert(1, "\"extra\":1,"), "\"p1\"", 7)));
    const char *error = nullptr;
    Doc out(preset_store_normalize(sent.json, &error));
    ASSERT_NE(out.json, nullptr) << error;
    EXPECT_EQ(preset_store_rev(out.json), 7);
    EXPECT_STREQ(cJSON_GetObjectItem(out.json, "default")->valuestring, "p1");
    cJSON *first = cJSON_GetArrayItem(cJSON_GetObjectItem(out.json, "presets"), 0);
    EXPECT_EQ(cJSON_GetObjectItem(first, "extra"), nullptr);
    EXPECT_STREQ(cJSON_GetObjectItem(first, "name")->valuestring, "人像");
    cJSON *params = cJSON_GetObjectItem(first, "params");
    EXPECT_NEAR(cJSON_GetObjectItem(params, "contrast")->valuedouble, 1.21, 1e-9);
    EXPECT_STREQ(cJSON_GetObjectItem(params, "ditherAlgorithm")->valuestring, "stucki");
}

TEST(PresetStore, RejectsBadDocuments)
{
    const std::string bad[] = {
        "[]",
        "{\"presets\":{}}",
        doc(preset("", "a", kParams)),
        doc(preset("p1", "", kParams)),
        doc(preset("p1", std::string(65, 'n'), kParams)),
        doc(preset(std::string(33, 'i'), "a", kParams)),
        doc(preset("p1", "a", "{\"exposure\":-1}")),
        doc(preset("p1", "a", "{\"exposure\":11}")),
        doc(preset("p1", "a", "{\"exposure\":\"1\"}")),
        doc(preset("p1", "a", "{\"toneMode\":\"loud\"}")),
        doc(preset("p1", "a", "{\"ditherAlgorithm\":\"atkinson\"}")),
        doc(preset("p1", "a", "{\"compressDynamicRange\":1}")),
        doc(preset("p1", "a", "[]")),
        doc(preset("p1", "a", kParams) + "," + preset("p1", "b", kParams)),
        doc(preset("p1", "a", kParams), "\"p2\""),
        doc(preset("p1", "a", kParams), "3"),
    };
    for (const auto &text : bad) {
        Doc sent(parse(text));
        const char *error = nullptr;
        Doc out(preset_store_normalize(sent.json, &error));
        EXPECT_EQ(out.json, nullptr) << text;
        EXPECT_NE(error, nullptr) << text;
    }
}

TEST(PresetStore, AtMostTwentyPresets)
{
    std::string list;
    for (int i = 0; i < 21; ++i) list += (i ? "," : "") + preset("p" + std::to_string(i), "n", "{}");
    Doc too_many(parse(doc(list)));
    EXPECT_EQ(preset_store_normalize(too_many.json, nullptr), nullptr);
    list = list.substr(0, list.rfind(",{"));
    Doc twenty(parse(doc(list)));
    Doc out(preset_store_normalize(twenty.json, nullptr));
    ASSERT_NE(out.json, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(cJSON_GetObjectItem(out.json, "presets")), 20);
}

TEST(PresetStore, SaveLoadAndInterruptedSave)
{
    std::string dir = make_dir();
    Doc missing(preset_store_load(dir.c_str()));
    EXPECT_EQ(cJSON_GetArraySize(cJSON_GetObjectItem(missing.json, "presets")), 0);

    Doc sent(parse(doc(preset("p1", "人像", kParams), "\"p1\"", 3)));
    Doc checked(preset_store_normalize(sent.json, nullptr));
    ASSERT_TRUE(preset_store_save(dir.c_str(), checked.json));
    EXPECT_FALSE(std::filesystem::exists(dir + "/.presets.json.new"));
    EXPECT_FALSE(std::filesystem::exists(dir + "/.presets.json.old"));
    Doc loaded(preset_store_load(dir.c_str()));
    EXPECT_EQ(preset_store_rev(loaded.json), 3);
    EXPECT_STREQ(cJSON_GetObjectItem(loaded.json, "default")->valuestring, "p1");

    // A power cut after parking the current copy leaves only ".old"
    std::filesystem::rename(dir + "/.presets.json", dir + "/.presets.json.old");
    Doc parked(preset_store_load(dir.c_str()));
    EXPECT_EQ(preset_store_rev(parked.json), 3);

    // A broken file reads as empty rather than failing
    std::ofstream(dir + "/.presets.json") << "{not json";
    std::filesystem::remove(dir + "/.presets.json.old");
    Doc broken(preset_store_load(dir.c_str()));
    EXPECT_EQ(cJSON_GetArraySize(cJSON_GetObjectItem(broken.json, "presets")), 0);

    preset_store_set_rev(checked.json, 4);
    EXPECT_EQ(preset_store_rev(checked.json), 4);
    std::filesystem::remove_all(dir);
}
