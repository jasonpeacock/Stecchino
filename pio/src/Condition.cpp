#include "Condition.h"

#include <Arduino.h>
#include <ArduinoLog.h>

#include "BatteryLevel.h"
#include "Configuration.h"
#include "LedStrip.h"
#include "Mpu.h"
#include "SleepNow.h"
#include "Stecchino.h"

Condition::Condition(LedStrip * led_strip, Mpu * mpu, BatteryLevel * battery_level)
    : state_(Stecchino::State::kCheckBattery), led_strip_(led_strip), mpu_(mpu), battery_level_(battery_level) {}

void Condition::Setup(void) {
    SetState(Stecchino::State::kCheckBattery);
}

void Condition::Update(const float                  angle_to_horizon,
                       const Stecchino::AccelStatus accel_status,
                       const Stecchino::Orientation orientation) {
    Log.trace(F("Condition::Update(): start\n"));

    switch (state_) {
        case Stecchino::State::kCheckBattery: {
            CheckBattery();
        } break;

        case Stecchino::State::kIdle: {
            Idle(accel_status, orientation);
        } break;

        case Stecchino::State::kStartPlayTransition: {
            StartPlayTransition();
        } break;

        case Stecchino::State::kPlay: {
            Play(accel_status);
        } break;

        case Stecchino::State::kGameOverTransition: {
            GameOverTransition();
        } break;

        case Stecchino::State::kSpiritLevel: {
            SpiritLevel(angle_to_horizon, orientation);
        } break;

        case Stecchino::State::kFakeSleep: {
            FakeSleep(angle_to_horizon);
        } break;

        case Stecchino::State::kSleepTransition: {
            SleepTransition();
        } break;
    }

    Log.trace(F("Condition::Update(): end\n"));
}

void Condition::SetState(const Stecchino::State state) {
    state_      = state;
    start_time_ = millis();
}

void Condition::CheckBattery(void) {
    Log.trace(F("Condition::CheckBattery(): start\n"));

    int vcc = battery_level_->GetMillivoltsForDisplay();
    led_strip_->ShowBatteryLevel(vcc);

    SetState(Stecchino::State::kIdle);

    Log.trace(F("Condition::CheckBattery(): end\n"));
}

void Condition::Idle(const Stecchino::AccelStatus accel_status, const Stecchino::Orientation orientation) {
    Log.trace(F("Condition::Idle(): start\n"));

    if (BUTTON_1_ON && ready_for_change_) {
        // delay(20); // debouncing
        ready_for_change_ = false;

        // Change light patterns when button is pressed.
        led_strip_->NextPattern();

        // Restart counter to enjoy new pattern longer.
        start_time_ = millis();
    } else if (!BUTTON_1_ON) {
        ready_for_change_ = true;
    }

    led_strip_->ShowPattern(LedStrip::Pattern::kIdle);

    if (millis() - start_time_ > IDLE_MS) {
        SetState(Stecchino::State::kFakeSleep);
    } else if (accel_status == Stecchino::AccelStatus::kStraight) {
        SetState(Stecchino::State::kStartPlayTransition);
    } else if (orientation == Stecchino::Orientation::kPosition_3) {
        SetState(Stecchino::State::kSpiritLevel);
    } else if (orientation == Stecchino::Orientation::kPosition_2) {
        SetState(Stecchino::State::kSleepTransition);
    }

    Log.trace(F("Condition::Idle(): end\n"));
}

void Condition::StartPlayTransition(void) {
    Log.trace(F("Condition::StartPlayTransition(): start\n"));

    /*
       if (millis() - start_time_ > START_PLAY_TRANSITION_MS) {
          previous_record_time_ = record_time_;

          SetState(Stecchino::State::kPlay);
       } else {
          led_strip_->ShowPattern(LedStrip::Pattern::kStartPlay);
       }
       */

    previous_record_time_ = record_time_;

    led_strip_->Off();

    SetState(Stecchino::State::kPlay);

    Log.trace(F("Condition::StartPlayTransition(): end\n"));
}

void Condition::Play(const Stecchino::AccelStatus accel_status) {
    Log.trace(F("Condition::Play(): start\n"));

    unsigned long elapsed_time = (millis() - start_time_) / 1000;

    if (elapsed_time > record_time_) {
        record_time_ = elapsed_time;
    }

    if (elapsed_time > previous_record_time_ && elapsed_time <= previous_record_time_ + 1 &&
        previous_record_time_ != 0) {
        led_strip_->ShowPattern(LedStrip::Pattern::kWahoo);
    }

    if (elapsed_time > MAX_PLAY_SECONDS) {
        state_      = Stecchino::State::kSleepTransition;
        start_time_ = millis();
    }

    if (NUM_LEDS_PER_SECONDS * static_cast<int>(elapsed_time) >= NUM_LEDS) {
        led_strip_->ShowPattern(LedStrip::Pattern::kWahoo);
    } else {
        led_strip_->On(NUM_LEDS_PER_SECONDS * static_cast<int>(elapsed_time),
                       NUM_LEDS_PER_SECONDS * static_cast<int>(record_time_));
    }

    if (accel_status == Stecchino::AccelStatus::kFallen) {
        state_      = Stecchino::State::kGameOverTransition;
        start_time_ = millis();
    }

    Log.trace(F("Condition::Play(): end\n"));
}

void Condition::GameOverTransition(void) {
    Log.trace(F("Condition::GameOverTransition(): start\n"));

    if (millis() - start_time_ > GAME_OVER_TRANSITION_MS) {
        state_      = Stecchino::State::kIdle;
        start_time_ = millis();
    } else {
        led_strip_->ShowPattern(LedStrip::Pattern::kGameOver);
    }

    Log.trace(F("Condition::GameOverTransition(): end\n"));
}

void Condition::SpiritLevel(const float angle_to_horizon, const Stecchino::Orientation orientation) {
    Log.trace(F("Condition::SpiritLevel(): start\n"));

    if (orientation == Stecchino::Orientation::kPosition_1 || orientation == Stecchino::Orientation::kPosition_2) {
        state_      = Stecchino::State::kIdle;
        start_time_ = millis();
    }

    if (millis() - start_time_ > MAX_SPIRIT_LEVEL_MS) {
        state_      = Stecchino::State::kFakeSleep;
        start_time_ = millis();
    }

    led_strip_->ShowSpiritLevel(angle_to_horizon);

    Log.trace(F("Condition::SpiritLevel(): end\n"));
}

void Condition::FakeSleep(const float angle_to_horizon) {
    Log.trace(F("Condition::FakeSleep(): start\n"));

    if (millis() - start_time_ > FAKE_SLEEP_MS) {
        SetState(Stecchino::State::kSleepTransition);
    }

    if (abs(angle_to_horizon) > 15) {
        SetState(Stecchino::State::kIdle);
    }

    if (BUTTON_1_ON) {
        ready_for_change_ = false;
        SetState(Stecchino::State::kIdle);
    } else {
        led_strip_->ShowPattern(LedStrip::Pattern::kOff);
    }

    Log.trace(F("Condition::FakeSleep(): end\n"));
}

void Condition::SleepTransition(void) {
    Log.trace(F("Condition::SleepTransition(): start\n"));

    if (millis() - start_time_ <= SLEEP_TRANSITION_MS) {
        led_strip_->ShowPattern(LedStrip::Pattern::kGoingToSleep);
        return;
    }

    // Go to sleep. This will continue when interrupted.
    sleepNow(interrupted);

    // position_->ClearSampleBuffer();

    // Restore MPU connection.
    Log.notice(F("Restarting MPU\n"));
    mpu_->Setup();

    start_time_ = millis();
    state_      = Stecchino::State::kCheckBattery;

    Log.trace(F("Condition::SleepTransition(): end\n"));
}
