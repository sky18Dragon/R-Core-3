#include <gtest/gtest.h>

extern "C" {
#include "frame_orientation.h"
}

TEST(FrameOrientation, GravityPicksTheFourPositions)
{
    EXPECT_EQ(frame_orientation_from_gravity(0, 1000), 0);
    EXPECT_EQ(frame_orientation_from_gravity(1000, 0), 90);
    EXPECT_EQ(frame_orientation_from_gravity(0, -1000), 180);
    EXPECT_EQ(frame_orientation_from_gravity(-1000, 0), 270);
    // Leaning back on a stand still reads upright.
    EXPECT_EQ(frame_orientation_from_gravity(60, 700), 0);
}

TEST(FrameOrientation, FlatOrSlantedIsUnknown)
{
    EXPECT_EQ(frame_orientation_from_gravity(30, 80), FRAME_ORIENTATION_UNKNOWN);    // flat
    EXPECT_EQ(frame_orientation_from_gravity(700, 700), FRAME_ORIENTATION_UNKNOWN);  // corner down
    EXPECT_EQ(frame_orientation_from_gravity(400, 499), FRAME_ORIENTATION_UNKNOWN);  // weak pull
}

TEST(FrameOrientation, SensorAxesMapOntoThePanel)
{
    const int16_t sample[3] = {-120, 30, 990};  // sensor x, y, z
    frame_axes_t axes = {2, -1, 0, 1};           // right = -z, down = +x
    int right = 0, down = 0;
    frame_gravity(&axes, sample, &right, &down);
    EXPECT_EQ(right, -990);
    EXPECT_EQ(down, -120);
}

TEST(FrameOrientation, ANewPositionCountsOnlyAfterItSettles)
{
    frame_orientation_tracker_t tracker;
    frame_orientation_tracker_init(&tracker, 0);
    EXPECT_FALSE(frame_orientation_feed(&tracker, 90, 1000));
    EXPECT_FALSE(frame_orientation_feed(&tracker, 90, 1000 + FRAME_ORIENTATION_SETTLE_MS - 1));
    EXPECT_TRUE(frame_orientation_feed(&tracker, 90, 1000 + FRAME_ORIENTATION_SETTLE_MS));
    EXPECT_EQ(tracker.settled, 90);
    EXPECT_FALSE(frame_orientation_feed(&tracker, 90, 9000));  // no repeat
    // A wobble back and forth restarts the wait.
    EXPECT_FALSE(frame_orientation_feed(&tracker, 0, 10000));
    EXPECT_FALSE(frame_orientation_feed(&tracker, 90, 10500));
    EXPECT_FALSE(frame_orientation_feed(&tracker, 0, 11000));
    EXPECT_FALSE(frame_orientation_feed(&tracker, 0, 11000 + FRAME_ORIENTATION_SETTLE_MS - 1));
    EXPECT_TRUE(frame_orientation_feed(&tracker, 0, 11000 + FRAME_ORIENTATION_SETTLE_MS));
    // Lying flat keeps the last position.
    EXPECT_FALSE(frame_orientation_feed(&tracker, FRAME_ORIENTATION_UNKNOWN, 20000));
    EXPECT_EQ(tracker.settled, 0);
}

TEST(FrameOrientation, UnknownStartTakesTheFirstSettledReading)
{
    frame_orientation_tracker_t tracker;
    frame_orientation_tracker_init(&tracker, FRAME_ORIENTATION_UNKNOWN);
    EXPECT_FALSE(frame_orientation_feed(&tracker, 270, 0));
    EXPECT_TRUE(frame_orientation_feed(&tracker, 270, FRAME_ORIENTATION_SETTLE_MS));
    EXPECT_EQ(tracker.settled, 270);
}
