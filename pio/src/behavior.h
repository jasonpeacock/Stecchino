#pragma once

#include "batteryLevel.h"
#include "ledStrip.h"
#include "mpu.h"
#include "stecchino.h"

class Behavior {
  public:
    Behavior(LedStrip * led_strip, Mpu * mpu, BatteryLevel * battery_level);

    void Setup(void);

    void Update(const float                  angle_to_horizon,
                const Stecchino::AccelStatus accel_status,
                const Stecchino::Orientation orientation);

    Stecchino::State GetState() const { return state_; };

  private:
    Stecchino::State previous_state_;
    Stecchino::State state_;

    volatile static bool interrupted_;

    LedStrip *     led_strip_;
    Mpu *          mpu_;
    BatteryLevel * battery_level_;

    // Track when the current state was entered so we can tell when it's time to
    // expire the state and transition to a new state.
    unsigned long start_time_ = 0;

    unsigned long record_time_          = 0;
    unsigned long previous_record_time_ = 0;
    bool          ready_for_change_     = false;

    void SetState(const Stecchino::State state);

    bool IsNewState(void) const;

    static void PinInterrupt(void);

    void Sleep(void);

    void CheckBattery(void);

    void Idle(const Stecchino::AccelStatus accel_status, const Stecchino::Orientation orientation);

    void StartPlayTransition(void);

    void Play(const Stecchino::AccelStatus accel_status);

    void GameOverTransition(void);

    void SpiritLevel(const float angle_to_horizon, const Stecchino::Orientation orientation);

    void FakeSleep(const float angle_to_horizon);

    void SleepTransition(void);
};
