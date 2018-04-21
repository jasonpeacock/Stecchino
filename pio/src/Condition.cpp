

#include "Condition.h"

#include <Arduino.h>
#include <ArduinoLog.h>

#include "BatteryLevel.h"
#include "Configuration.h"
#include "LedPatterns.h"
#include "LedStrip.h"
#include "Mpu.h"
#include "Position.h"
#include "SleepNow.h"
#include "SpiritLevel.h"

Condition::Condition(LedStrip* led_strip, Mpu* mpu, BatteryLevel* battery_level) : led_strip_(led_strip), mpu_(mpu), battery_level_(battery_level) {}

void Condition::Setup(void) {
  start_time_ = millis();
}

void Condition::Update(float angle_to_horizon, Position::AccelStatus accel_status, Position::Orientation orientation) {
  Log.trace(F("Condition::Update(): start\n"));

  switch (state_) {
    case State::kCount:
      break;

    case State::kCheckBattery: {
      battery_level_->Show();
      delay(2000);
      led_strip_->Off();

      state_ = State::kIdle;
    } break;

    case State::kIdle: {
      if (BUTTON_1_ON && ready_for_change_) {
        //delay(20); // debouncing
        ready_for_change_ = false;

        // Change light patterns when button is pressed.
        nextPattern();

        // Restart counter to enjoy new pattern longer.
        start_time_ = millis();
      }

      if (!BUTTON_1_ON) {
        ready_for_change_ = true;
      }

      if (millis() - start_time_ > IDLE_MS) {
        state_      = State::kFakeSleep;
        start_time_ = millis();
      }

      if (accel_status == Position::AccelStatus::kStraight) {
        state_      = State::kStartPlayTransition;
        start_time_ = millis();
      }

      if (orientation == Position::Orientation::kPosition_3) {
        state_      = State::kSpiritLevel;
        start_time_ = millis();
      }

      if (orientation == Position::Orientation::kPosition_2) {
        state_      = State::kSleepTransition;
        start_time_ = millis();
      } else {
        led(leds, Pattern::kIdle);
      }
    } break;

    case State::kStartPlayTransition: {
      /*
                       if (millis() - start_time_ > START_PLAY_TRANSITION_MS) {
                       state_ = State::kPlay;
                       start_time_ = millis();
                       previous_record_time_ = record_time_;
                       } else {
                       led(leds, Pattern::kStartPlay);
                       }
                       */

      led_strip_->Off();
      start_time_           = millis();
      previous_record_time_ = record_time_;

      state_ = State::kPlay;
    } break;

    case State::kPlay: {
      unsigned long elapsed_time = (millis() - start_time_) / 1000;

      if (elapsed_time > record_time_) {
        record_time_ = elapsed_time;
      }

      if (elapsed_time > previous_record_time_ &&
          elapsed_time <= previous_record_time_ + 1 &&
          previous_record_time_ != 0) {
        led(leds, Pattern::kWahoo);
      }

      if (elapsed_time > MAX_PLAY_SECONDS) {
        state_      = State::kSleepTransition;
        start_time_ = millis();
      }

      if (NUM_LEDS_PER_SECONDS * static_cast<int>(elapsed_time) >= NUM_LEDS) {
        led(leds, Pattern::kWahoo);
      } else {
        ledsOn(leds,
               NUM_LEDS_PER_SECONDS * static_cast<int>(elapsed_time),
               NUM_LEDS_PER_SECONDS * static_cast<int>(record_time_));
      }

      if (accel_status == Position::AccelStatus::kFallen) {
        state_      = State::kGameOverTransition;
        start_time_ = millis();
      }
    } break;

    case State::kGameOverTransition: {
      if (millis() - start_time_ > GAME_OVER_TRANSITION_MS) {
        state_      = State::kIdle;
        start_time_ = millis();
      } else {
        led(leds, Pattern::kGameOver);
      }
    }

    break;

    case State::kSpiritLevel: {
      if (orientation == Position::Orientation::kPosition_1 || orientation == Position::Orientation::kPosition_2) {
        state_      = State::kIdle;
        start_time_ = millis();
      }

      if (millis() - start_time_ > MAX_SPIRIT_LEVEL_MS) {
        state_      = State::kFakeSleep;
        start_time_ = millis();
      }

      spiritLevel(leds, angle_to_horizon);
    }

    break;

    case State::kFakeSleep: {
      if (millis() - start_time_ > FAKE_SLEEP_MS) {
        state_      = State::kSleepTransition;
        start_time_ = millis();
      }

      if (abs(angle_to_horizon) > 15) {
        state_      = State::kIdle;
        start_time_ = millis();
      }

      if (BUTTON_1_ON) {
        state_            = State::kIdle;
        ready_for_change_ = false;
        start_time_       = millis();
      } else {
        led(leds, Pattern::kOff);
      }
    }

    break;

    case State::kSleepTransition: {
      if (millis() - start_time_ > SLEEP_TRANSITION_MS) {
        // Go to sleep. This will continue when interrupted.
        sleepNow(interrupted);

        //position_->ClearSampleBuffer();

        // Restore MPU connection.
        Log.notice(F("Restarting MPU\n"));
        mpu_->Setup();

        state_      = State::kCheckBattery;
        start_time_ = millis();
      } else {
        led(leds, Pattern::kGoingToSleep);
      }
    }

    break;
  }

  Log.trace(F("Condition::Update(): end\n"));
}
