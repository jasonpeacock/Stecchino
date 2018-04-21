#pragma once

#include "BatteryLevel.h"
#include "LedStrip.h"
#include "Mpu.h"
#include "Position.h"

class Condition {
 public:
  Condition(LedStrip* led_strip, Mpu* mpu, BatteryLevel* battery_level);

  void Setup(void);

  void Update(float angle_to_horizon, Position::AccelStatus accel_status, Position::Orientation orientation);

 private:
  enum class State {
    kCheckBattery,
    kFakeSleep,
    kGameOverTransition,
    kIdle,
    kPlay,
    kSleepTransition,
    kSpiritLevel,
    kStartPlayTransition,
    kCount,
  };

  // Display battery level at startup as the initial state.
  State state_ = State::kCheckBattery;

  LedStrip*     led_strip_;
  Mpu*          mpu_;
  BatteryLevel* battery_level_;

  unsigned long start_time_           = 0;
  unsigned long record_time_          = 0;
  unsigned long previous_record_time_ = 0;
  bool          ready_for_change_     = false;
};
