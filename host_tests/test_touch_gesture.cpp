#include <gtest/gtest.h>

extern "C" {
#include "touch_gesture.h"
}

namespace
{
struct Feed {
    touch_tracker_t t;
    touch_gesture_t g{};
    Feed() { touch_tracker_reset(&t); }
    bool down(uint16_t x, uint16_t y, int64_t ms) { return touch_tracker_feed(&t, true, 1, x, y, ms, &g); }
    bool up(int64_t ms) { return touch_tracker_feed(&t, false, 0, 0, 0, ms, &g); }
    // Straight drag from (x0,y0) to (x1,y1) in steps, then release.
    touch_gesture_t drag(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int64_t ms = 300)
    {
        EXPECT_FALSE(down(x0, y0, 0));
        for (int i = 1; i <= 5; ++i)
            EXPECT_FALSE(down(x0 + (x1 - x0) * i / 5, y0 + (y1 - y0) * i / 5, ms * i / 5));
        EXPECT_TRUE(up(ms + 20));
        return g;
    }
};
}  // namespace

TEST(TouchGesture, SwipesClassifyByDominantNativeAxis)
{
    EXPECT_EQ(Feed().drag(300, 800, 300, 500).kind, TOUCH_GESTURE_SWIPE_Y_NEG);
    EXPECT_EQ(Feed().drag(300, 500, 300, 800).kind, TOUCH_GESTURE_SWIPE_Y_POS);
    EXPECT_EQ(Feed().drag(500, 600, 200, 650).kind, TOUCH_GESTURE_SWIPE_X_NEG);
    EXPECT_EQ(Feed().drag(200, 600, 500, 560).kind, TOUCH_GESTURE_SWIPE_X_POS);
}

TEST(TouchGesture, ShortOrSlowOrWanderingMovesAreNotSwipes)
{
    EXPECT_EQ(Feed().drag(300, 600, 300, 600 + TOUCH_SWIPE_MIN_PX - 1).kind, TOUCH_GESTURE_CANCEL);
    EXPECT_EQ(Feed().drag(300, 400, 300, 900, TOUCH_SWIPE_MAX_MS + 100).kind, TOUCH_GESTURE_CANCEL);
    EXPECT_EQ(Feed().drag(300, 600, 300 + 10, 600 + 10).kind, TOUCH_GESTURE_TAP);
    // Leaving the slop and returning never restores tap eligibility.
    Feed f;
    f.down(300, 600, 0); f.down(300, 680, 100); f.down(300, 600, 200);
    ASSERT_TRUE(f.up(250));
    EXPECT_EQ(f.g.kind, TOUCH_GESTURE_CANCEL);
}

TEST(TouchGesture, KeysUseOriginStripAndPitch)
{
    for (int k = 0; k < TOUCH_KEY_COUNT; ++k) {
        Feed f;
        f.down(k * TOUCH_KEY_PITCH + 80, TOUCH_KEY_AREA_TOP + 20, 0);
        ASSERT_TRUE(f.up(100));
        EXPECT_EQ(f.g.kind, TOUCH_GESTURE_KEY);
        EXPECT_EQ(f.g.key, k);
    }
    Feed beyond;  // past the third key segment
    beyond.down(TOUCH_KEY_COUNT * TOUCH_KEY_PITCH + 5, TOUCH_KEY_AREA_TOP + 20, 0);
    ASSERT_TRUE(beyond.up(100));
    EXPECT_EQ(beyond.g.kind, TOUCH_GESTURE_CANCEL);
    // A swipe that starts on the key strip is not a screen swipe.
    EXPECT_EQ(Feed().drag(80, TOUCH_KEY_AREA_TOP + 10, 80, TOUCH_KEY_AREA_TOP - 400).kind,
              TOUCH_GESTURE_CANCEL);
}

TEST(TouchGesture, MultiTouchAndClockRegressionCancel)
{
    Feed f;
    f.down(300, 600, 0);
    ASSERT_TRUE(touch_tracker_feed(&f.t, true, 2, 300, 700, 50, &f.g));
    EXPECT_EQ(f.g.kind, TOUCH_GESTURE_CANCEL);
    EXPECT_FALSE(touch_tracker_active(&f.t));
    Feed r;
    r.down(300, 600, 100);
    ASSERT_TRUE(r.down(300, 400, 50));
    EXPECT_EQ(r.g.kind, TOUCH_GESTURE_CANCEL);
    // Idle multi-touch or release produces nothing.
    Feed idle;
    EXPECT_FALSE(touch_tracker_feed(&idle.t, true, 2, 1, 1, 0, &idle.g));
    EXPECT_FALSE(idle.up(0));
}

TEST(TouchGesture, EmptyZeroSlotIsIgnored)
{
    Feed f;
    EXPECT_FALSE(f.down(0, 0, 0));
    EXPECT_FALSE(touch_tracker_active(&f.t));
    f.down(300, 800, 10);
    f.down(0, 0, 50);  // must not count as a drag to the origin
    f.down(300, 500, 200);
    ASSERT_TRUE(f.up(260));
    EXPECT_EQ(f.g.kind, TOUCH_GESTURE_SWIPE_Y_NEG);
    EXPECT_EQ(f.g.x1, 300); EXPECT_EQ(f.g.y1, 500);
}

TEST(TouchGesture, ActionMappingFollowsLayoutAndRotation)
{
    touch_gesture_t g{};
    g.kind = TOUCH_GESTURE_SWIPE_Y_NEG;
    EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_NEXT);
    EXPECT_EQ(touch_action_for(&g, true, true), TOUCH_ACTION_PREVIOUS);
    g.kind = TOUCH_GESTURE_SWIPE_Y_POS;
    EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_PREVIOUS);
    EXPECT_EQ(touch_action_for(&g, true, true), TOUCH_ACTION_NEXT);
    g.kind = TOUCH_GESTURE_SWIPE_X_NEG;  // vertical in landscape: ignored
    EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_NONE);
    EXPECT_EQ(touch_action_for(&g, false, false), TOUCH_ACTION_NEXT);
    g.kind = TOUCH_GESTURE_TAP;
    EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_NONE);
    g.kind = TOUCH_GESTURE_KEY;
    g.key = 0; EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_PREVIOUS);
    g.key = 1; EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_NONE);
    g.key = 2; EXPECT_EQ(touch_action_for(&g, true, false), TOUCH_ACTION_NEXT);
    g.key = 2; EXPECT_EQ(touch_action_for(&g, true, true), TOUCH_ACTION_PREVIOUS);
    EXPECT_EQ(touch_action_for(nullptr, true, false), TOUCH_ACTION_NONE);
    EXPECT_STREQ(touch_action_name(TOUCH_ACTION_NEXT), "next");
    EXPECT_STREQ(touch_gesture_name((touch_gesture_kind_t) 99), "unknown");
}
