#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "image_history.h"
}

namespace
{
std::string back(image_history_t &h)
{
    char out[IMAGE_HISTORY_PATH_MAX];
    return image_history_back(&h, out, sizeof(out)) ? std::string(out) : std::string("<none>");
}
}  // namespace

TEST(ImageHistory, BackWalksToEarlierImagesAndReshowIsNotAStep)
{
    image_history_t h;
    image_history_reset(&h);
    EXPECT_EQ(back(h), "<none>");
    image_history_push(&h, "/a");
    EXPECT_EQ(back(h), "<none>");
    image_history_push(&h, "/b");
    image_history_push(&h, "/b");
    image_history_push(&h, "/c");
    EXPECT_EQ(back(h), "/b");
    image_history_push(&h, "/b");  // showing the previous image re-records it
    EXPECT_EQ(back(h), "/a");
    EXPECT_EQ(back(h), "<none>");
    EXPECT_EQ(h.count, 1u);
}

TEST(ImageHistory, FullHistoryDropsOldest)
{
    image_history_t h;
    image_history_reset(&h);
    for (int i = 0; i < IMAGE_HISTORY_DEPTH + 5; ++i)
        image_history_push(&h, ("/p" + std::to_string(i)).c_str());
    EXPECT_EQ(h.count, (unsigned) IMAGE_HISTORY_DEPTH);
    for (int i = IMAGE_HISTORY_DEPTH + 3; i >= 5; --i)
        EXPECT_EQ(back(h), "/p" + std::to_string(i));
    EXPECT_EQ(back(h), "<none>");
}

TEST(ImageHistory, RejectsInvalidInputAndSmallOutput)
{
    image_history_t h;
    image_history_reset(&h);
    image_history_push(&h, nullptr);
    image_history_push(&h, "");
    image_history_push(&h, std::string(IMAGE_HISTORY_PATH_MAX, 'x').c_str());
    EXPECT_EQ(h.count, 0u);
    image_history_push(&h, "/long-enough");
    image_history_push(&h, "/b");
    char tiny[4];
    EXPECT_FALSE(image_history_back(&h, tiny, sizeof(tiny)));
    EXPECT_EQ(h.count, 2u);
    EXPECT_FALSE(image_history_back(nullptr, tiny, sizeof(tiny)));
}

TEST(ImageHistory, ForgetRemovesDeletedFileAndMergesNeighbours)
{
    image_history_t h;
    image_history_reset(&h);
    for (const char *p : {"/a", "/x", "/a", "/b", "/x"}) image_history_push(&h, p);
    image_history_forget(&h, "/x");
    EXPECT_EQ(h.count, 2u);  // a, a merged; b
    EXPECT_EQ(back(h), "/a");
    EXPECT_EQ(back(h), "<none>");
    // Works across the ring wrap.
    image_history_reset(&h);
    for (int i = 0; i < IMAGE_HISTORY_DEPTH + 3; ++i)
        image_history_push(&h, i % 2 ? "/odd" : ("/e" + std::to_string(i)).c_str());
    image_history_forget(&h, "/odd");
    EXPECT_EQ(h.count, (unsigned) IMAGE_HISTORY_DEPTH / 2);
    EXPECT_EQ(back(h), "/e16");
    image_history_forget(&h, "/e16");
    image_history_forget(&h, "/missing");
    EXPECT_EQ(back(h), "/e12");
}
