#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Which way the frame stands, from gravity (docs/PRD-PHOTO-ORIENTATION.md §6).
// Orientation: how far the frame is turned clockwise from its normal landscape
// position, 0 / 90 / 180 / 270; FRAME_ORIENTATION_UNKNOWN when lying flat or
// held at a slant. In panel terms (x right, y down in normal landscape),
// gravity points along +y at 0, +x at 90, -y at 180 and -x at 270.
#define FRAME_ORIENTATION_UNKNOWN (-1)
// A new orientation counts once it has held this long.
#define FRAME_ORIENTATION_SETTLE_MS 2000

// Where each panel axis comes from on the sensor: axis 0/1/2 = sensor x/y/z,
// sign +1/-1. Found by holding the frame in each position once.
typedef struct {
    uint8_t right_axis;
    int8_t right_sign;
    uint8_t down_axis;
    int8_t down_sign;
} frame_axes_t;

// Gravity along the panel's right and down axes, in milli-g.
void frame_gravity(const frame_axes_t *axes, const int16_t sensor_mg[3], int *right_mg,
                   int *down_mg);
// Orientation for a gravity reading, or FRAME_ORIENTATION_UNKNOWN when the
// frame is near flat (the in-plane pull is weak) or between two positions.
int frame_orientation_from_gravity(int right_mg, int down_mg);

typedef struct {
    int settled;    // last orientation that held long enough, or UNKNOWN
    int candidate;  // what the readings say now
    int64_t since_ms;
} frame_orientation_tracker_t;

void frame_orientation_tracker_init(frame_orientation_tracker_t *tracker, int initial);
// Feeds one reading; returns true when the settled orientation changes. Unknown
// readings (flat, slanted) never change it: the frame keeps its last position.
bool frame_orientation_feed(frame_orientation_tracker_t *tracker, int reading, int64_t now_ms);
#ifdef __cplusplus
}
#endif
