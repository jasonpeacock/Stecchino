#include <Arduino.h>

#include <MPU6050.h>
#include <RunningMedian.h>

#include "ArduinoLog.h"
#include "Configuration.h"

int16_t ax;
int16_t ay;
int16_t az;
int16_t gx;
int16_t gy;
int16_t gz;

enum class AccelStatus {
  kFallen,
  kStraight,
  kUnknown,
};

AccelStatus accel_status = AccelStatus::kUnknown;

RunningMedian a_forward_rolling_sample  = RunningMedian(5);
RunningMedian a_sideway_rolling_sample  = RunningMedian(5);
RunningMedian a_vertical_rolling_sample = RunningMedian(5);

int a_forward_offset  = 0;
int a_sideway_offset  = 0;
int a_vertical_offset = 0;

// Used to detect position of buttons relative to Stecchino and user
enum class Position {
  kNone,
  kPosition_1,
  kPosition_2,
  kPosition_3,
  kPosition_4,
  kPosition_5,
  kPosition_6,
  kCount,
};

Position orientation = Position::kNone;

// POSITION_1: Stecchino V3/V4 horizontal with buttons up (idle)
// POSITION_2: Stecchino V3/V4 horizontal with buttons down (force sleep)
// POSITION_3: Stecchino V3/V4 horizontal with long edge down (spirit level)
// POSITION_4: Stecchino V3/V4 horizontal with short edge down (opposite to spirit level)
// POSITION_5: Stecchino V3/V4 vertical with PCB up (normal game position = straight)
// POSITION_6: Stecchino V3/V4 vertical with PCB down (easy game position = straight)
const char* kPositionNames[static_cast<int>(Position::kCount)] = {
  "None",
  "POSITION_1",
  "POSITION_2",
  "POSITION_3",
  "POSITION_4",
  "POSITION_5",
  "POSITION_6",
};

// Reads acceleration from MPU6050 to evaluate current condition.
// Tunables:
// Output values: accel_status=fallen or straight
//                orientation=POSITION_1 to POSITION_6
//                angle_to_horizon
float checkPosition(MPU6050 mpu) {
  Log.trace(F("checkPosition(): start\n"));

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float angle_to_horizon = 0;

  // Offset accel readings
  //int a_forward_offset = -2;
  //int a_sideway_offset = 2;
  //int a_vertical_offset = 1;

  // Convert to expected orientation - includes unit conversion to "cents of g" for MPU range set to 2g
  int a_forward  = (ACCELEROMETER_ORIENTATION == 0 ? ax : (ACCELEROMETER_ORIENTATION == 1 ? ay : az)) / 164;
  int a_sideway  = (ACCELEROMETER_ORIENTATION == 0 ? ay : (ACCELEROMETER_ORIENTATION == 1 ? az : ax)) / 164;
  int a_vertical = (ACCELEROMETER_ORIENTATION == 0 ? az : (ACCELEROMETER_ORIENTATION == 1 ? ax : ay)) / 164;

  // Update rolling average for smoothing
  a_forward_rolling_sample.add(a_forward);
  a_sideway_rolling_sample.add(a_sideway);
  a_vertical_rolling_sample.add(a_vertical);

  // Get median
  int a_forward_rolling_sample_median  = a_forward_rolling_sample.getMedian() - a_forward_offset;
  int a_sideway_rolling_sample_median  = a_sideway_rolling_sample.getMedian() - a_sideway_offset;
  int a_vertical_rolling_sample_median = a_vertical_rolling_sample.getMedian() - a_vertical_offset;

  // Evaluate current condition based on smoothed accelarations
  accel_status = AccelStatus::kUnknown;
  if (abs(a_sideway_rolling_sample_median) > abs(a_vertical_rolling_sample_median) ||
      abs(a_forward_rolling_sample_median) > abs(a_vertical_rolling_sample_median)) {
    accel_status = AccelStatus::kFallen;
  } else if (abs(a_sideway_rolling_sample_median) < abs(a_vertical_rolling_sample_median) &&
             abs(a_forward_rolling_sample_median) < abs(a_vertical_rolling_sample_median)) {
    accel_status = AccelStatus::kStraight;
  }

  if (a_vertical_rolling_sample_median >= 80 &&
      abs(a_forward_rolling_sample_median) <= 25 &&
      abs(a_sideway_rolling_sample_median) <= 25 &&
      orientation != Position::kPosition_6) {
    // coté 1 en haut
    orientation = Position::kPosition_6;
  } else if (a_forward_rolling_sample_median >= 80 &&
             abs(a_vertical_rolling_sample_median) <= 25 &&
             abs(a_sideway_rolling_sample_median) <= 25 &&
             orientation != Position::kPosition_2) {
    // coté 2 en haut
    orientation = Position::kPosition_2;
  } else if (a_vertical_rolling_sample_median <= -80 &&
             abs(a_forward_rolling_sample_median) <= 25 &&
             abs(a_sideway_rolling_sample_median) <= 25 &&
             orientation != Position::kPosition_5) {
    // coté 3 en haut
    orientation = Position::kPosition_5;
  } else if (a_forward_rolling_sample_median <= -80 &&
             abs(a_vertical_rolling_sample_median) <= 25 &&
             abs(a_sideway_rolling_sample_median) <= 25 &&
             orientation != Position::kPosition_1) {
    // coté 4 en haut
    orientation = Position::kPosition_1;
  } else if (a_sideway_rolling_sample_median >= 80 &&
             abs(a_vertical_rolling_sample_median) <= 25 &&
             abs(a_forward_rolling_sample_median) <= 25 &&
             orientation != Position::kPosition_3) {
    // coté LEDs en haut
    orientation = Position::kPosition_3;
  } else if (a_sideway_rolling_sample_median <= -80 &&
             abs(a_vertical_rolling_sample_median) <= 25 &&
             abs(a_forward_rolling_sample_median) <= 25 &&
             orientation != Position::kPosition_4) {
    // coté batteries en haut
    orientation = Position::kPosition_4;
  } else {
    // TODO throw an error?
  }

  angle_to_horizon = atan2(
                         float(a_vertical_rolling_sample_median),
                         float(max(abs(a_sideway_rolling_sample_median),
                                   abs(a_forward_rolling_sample_median)))) *
                     180 / PI;

  Log.notice(
      "a_forward: %d a_sideway: %d a_vertical: %d - %s - %d - %F\n",
      a_forward_rolling_sample_median,
      a_sideway_rolling_sample_median,
      a_vertical_rolling_sample_median,
      kPositionNames[static_cast<int>(orientation)],
      accel_status,
      angle_to_horizon);

  Log.trace(F("checkPosition(): end\n"));

  return angle_to_horizon;
}
