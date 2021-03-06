#pragma once

#include <Arduino.h>
#include <RunningMedian.h>

#include "configuration.h"
#include "mpu.h"
#include "stecchino.h"

class Position {
  public:
    Position(Mpu * mpu);

    void Setup(void);

    void Update(void);

    void ClearSampleBuffer(void);

    Stecchino::AccelStatus GetAccelStatus(void) const { return accel_status_; }

    Stecchino::Orientation GetOrientation(void) const { return orientation_; }

    float GetAngleToHorizon(void) const { return angle_to_horizon_; }

  private:
    // Offset accel readings
    // const int kForwardOffset = -2;
    // const int kSidewayOffset = 2;
    // const int kVerticalOffset = 1;

    const float kForwardOffset  = 0.;
    const float kSidewayOffset  = 0.;
    const float kVerticalOffset = 0.;

    const uint8_t kRunningMedianBufferSize = 5;

    // Unit conversion to "cents of g" for MPU range set to 2g
    const float kMpuUnitConversion_2g = 164.;

    const uint8_t kAccelOrientation = ACCELEROMETER_ORIENTATION;

    float angle_to_horizon_ = 0.;

    RunningMedian forward_rolling_sample_  = RunningMedian(kRunningMedianBufferSize);
    RunningMedian sideway_rolling_sample_  = RunningMedian(kRunningMedianBufferSize);
    RunningMedian vertical_rolling_sample_ = RunningMedian(kRunningMedianBufferSize);

    Stecchino::AccelStatus accel_status_ = Stecchino::AccelStatus::kUnknown;
    Stecchino::Orientation orientation_  = Stecchino::Orientation::kUnknown;

    Mpu * mpu_;
};
