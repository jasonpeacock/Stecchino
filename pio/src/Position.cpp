#include "Position.h"

#include <Arduino.h>
#include <RunningMedian.h>

#include "ArduinoLog.h"
#include "Mpu.h"

Position::Position(Mpu* mpu) : mpu_(mpu) {}

void Position::Setup(void) {}

// Reads acceleration from MPU6050 to evaluate current condition.
//
// Tunables:
//
// Output values: accel_status=fallen or straight
//                orientation=POSITION_1 to POSITION_6
//                angle_to_horizon
void Position::Update(void) {
  Log.trace(F("Update(): start\n"));

  int16_t ax = 0;
  int16_t ay = 0;
  int16_t az = 0;

  mpu_->GetAccelMotion(&ax, &ay, &az);

  // Convert to expected orientation.
  float forward_accel  =
    static_cast<float>(kAccelOrientation == 0 ? ax : (kAccelOrientation == 1 ? ay : az)) / kMpuUnitConversion_2g;
  float sideway_accel  =
    static_cast<float>(kAccelOrientation == 0 ? ay : (kAccelOrientation == 1 ? az : ax)) / kMpuUnitConversion_2g;
  float vertical_accel =
    static_cast<float>(kAccelOrientation == 0 ? az : (kAccelOrientation == 1 ? ax : ay)) / kMpuUnitConversion_2g;

  forward_rolling_sample_.add(forward_accel);
  sideway_rolling_sample_.add(sideway_accel);
  vertical_rolling_sample_.add(vertical_accel);

  float forward_rolling_sample_median  = forward_rolling_sample_.getMedian() - kForwardOffset;
  float sideway_rolling_sample_median  = sideway_rolling_sample_.getMedian() - kSidewayOffset;
  float vertical_rolling_sample_median = vertical_rolling_sample_.getMedian() - kVerticalOffset;

  accel_status_ = AccelStatus::kUnknown;

  // Evaluate current condition based on smoothed accelarations
  if (abs(sideway_rolling_sample_median) > abs(vertical_rolling_sample_median) ||
      abs(forward_rolling_sample_median) > abs(vertical_rolling_sample_median)) {
    accel_status_ = AccelStatus::kFallen;
  } else if (abs(sideway_rolling_sample_median) < abs(vertical_rolling_sample_median) &&
             abs(forward_rolling_sample_median) < abs(vertical_rolling_sample_median)) {
    accel_status_ = AccelStatus::kStraight;
  }

  if (vertical_rolling_sample_median >= 80 &&
      abs(forward_rolling_sample_median) <= 25 &&
      abs(sideway_rolling_sample_median) <= 25) {
    // Stecchino vertical with PCB down (easy game position = straight)
    Log.notice(F("Orientation: Position 6\n"));
    orientation_ = Orientation::kPosition_6;
  } else if (forward_rolling_sample_median >= 80 &&
             abs(vertical_rolling_sample_median) <= 25 &&
             abs(sideway_rolling_sample_median) <= 25) {
    // Stecchino horizontal with buttons down (force sleep)
    Log.notice(F("Orientation: Position 2\n"));
    orientation_ = Orientation::kPosition_2;
  } else if (vertical_rolling_sample_median <= -80 &&
             abs(forward_rolling_sample_median) <= 25 &&
             abs(sideway_rolling_sample_median) <= 25) {
    // Stecchino vertical with PCB up (normal game position = straight)
    Log.notice(F("Orientation: Position 5\n"));
    orientation_ = Orientation::kPosition_5;
  } else if (forward_rolling_sample_median <= -80 &&
             abs(vertical_rolling_sample_median) <= 25 &&
             abs(sideway_rolling_sample_median) <= 25) {
    // Stecchino horizontal with buttons up (idle)
    Log.notice(F("Orientation: Position 1\n"));
    orientation_ = Orientation::kPosition_1;
  } else if (sideway_rolling_sample_median >= 80 &&
             abs(vertical_rolling_sample_median) <= 25 &&
             abs(forward_rolling_sample_median) <= 25) {
    // Stecchino horizontal with long edge down (spirit level)
    Log.notice(F("Orientation: Position 3\n"));
    orientation_ = Orientation::kPosition_3;
  } else if (sideway_rolling_sample_median <= -80 &&
             abs(vertical_rolling_sample_median) <= 25 &&
             abs(forward_rolling_sample_median) <= 25) {
    // Stecchino horizontal with short edge down (opposite to spirit level)
    Log.notice(F("Orientation: Position 4\n"));
    orientation_ = Orientation::kPosition_4;
  } else {
    // TODO throw an error?
  }

  angle_to_horizon_ = atan2(
                         float(vertical_rolling_sample_median),
                         float(max(abs(sideway_rolling_sample_median),
                                   abs(forward_rolling_sample_median)))) *
                     180 / PI;

  Log.notice(
      "Forward: %d Sideway: %d Vertical: %d - %d - %F\n",
      forward_rolling_sample_median,
      sideway_rolling_sample_median,
      vertical_rolling_sample_median,
      accel_status_,
      angle_to_horizon_);

  Log.trace(F("Update(): end\n"));
}

// Clear running median buffer.
void Position::ClearSampleBuffer(void) {
  forward_rolling_sample_.clear();
  sideway_rolling_sample_.clear();
  vertical_rolling_sample_.clear();
}

Position::AccelStatus Position::GetAccelStatus(void) {
  return accel_status_;
}

Position::Orientation Position::GetOrientation(void) {
  return orientation_;
}

float Position::GetAngleToHorizon(void) {
  return angle_to_horizon_;
}
