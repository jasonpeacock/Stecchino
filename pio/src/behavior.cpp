#include "behavior.h"

#include <Arduino.h>
#include <ArduinoLog.h>
#include <avr/sleep.h>

#include "batteryLevel.h"
#include "configuration.h"
#include "ledStrip.h"
#include "mpu.h"
#include "stecchino.h"

volatile bool Behavior::interrupted_ = false;

Behavior::Behavior(LedStrip * led_strip, Mpu * mpu, BatteryLevel * battery_level)
    : state_(Stecchino::State::kUnknown), led_strip_(led_strip), mpu_(mpu), battery_level_(battery_level) {}

void Behavior::Setup(void) {
    SetState(Stecchino::State::kCheckBattery);
}

void Behavior::Update(const float                  angle_to_horizon,
                      const Stecchino::AccelStatus accel_status,
                      const Stecchino::Orientation orientation) {
    Log.trace(F("Behavior::Update\n"));

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

        default: {
            // If we don't know what to do, check the battery.
            Log.error(F("Unknown state [%d], defaulting to CheckBattery\n"), static_cast<int>(state_));
            SetState(Stecchino::State::kCheckBattery);
        } break;
    }
}

void Behavior::SetState(const Stecchino::State state) {
    previous_state_ = state_;
    state_          = state;
    start_time_     = millis();

    led_strip_->Off();
}

bool Behavior::IsNewState(void) const {
    return (state_ == previous_state_);
}

void Behavior::PinInterrupt(void) {
    Log.trace(F("PinInterrupt()\n"));

    if (Behavior::interrupted_) {
        Log.notice(F("Interrupted, disabling interrupt\n"));
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    }
}

void Behavior::Sleep(void) {
    Log.trace(F("Behavior::Sleep()\n"));

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    Behavior::interrupted_ = false;
    attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), PinInterrupt, LOW);

    Log.trace(F("powering down\n"));

    // Put the device to sleep:
    digitalWrite(PIN_MOSFET_GATE, LOW);  // Turn LEDs off to indicate sleep.

    // Turn MPU off.
    // XXX digitalWrite(PIN_MPU_POWER, LOW);
    delay(100);  // XXX needed?

    Log.trace(F("sleeping\n"));

    Serial.flush();
    sleep_mode();

    // Upon waking up, sketch continues from this point.
    Log.trace(F("woke\n"));
    Behavior::interrupted_ = true;
    sleep_disable();

    // Turn LEDs on to indicate awake.
    digitalWrite(PIN_MOSFET_GATE, HIGH);

    // Turn MPU on.
    // XXX digitalWrite(PIN_MPU_POWER, HIGH);

    delay(100);  // XXX needed?
}
void Behavior::CheckBattery(void) {
    Log.trace(F("Behavior::CheckBattery\n"));

    if (millis() - start_time_ > MAX_SHOW_BATTERY_MS) {
        SetState(Stecchino::State::kIdle);
        return;
    }

    int vcc = battery_level_->GetMillivoltsForDisplay();
    led_strip_->ShowBatteryLevel(vcc);
}

void Behavior::Idle(const Stecchino::AccelStatus accel_status, const Stecchino::Orientation orientation) {
    Log.trace(F("Behavior::Idle\n"));

    if (millis() - start_time_ > MAX_IDLE_MS) {
        SetState(Stecchino::State::kFakeSleep);
        return;
    }

    if (accel_status == Stecchino::AccelStatus::kStraight) {
        SetState(Stecchino::State::kStartPlayTransition);
        return;
    }

    if (orientation == Stecchino::Orientation::kPosition_3) {
        SetState(Stecchino::State::kSpiritLevel);
        return;
    }

    if (orientation == Stecchino::Orientation::kPosition_2) {
        SetState(Stecchino::State::kSleepTransition);
        return;
    }

    led_strip_->ShowIdle();
}

void Behavior::StartPlayTransition(void) {
    Log.trace(F("Behavior::StartPlayTransition\n"));

    if (millis() - start_time_ > MAX_START_PLAY_TRANSITION_MS) {
        previous_record_time_ = record_time_;

        SetState(Stecchino::State::kPlay);
        return;
    }

    led_strip_->ShowStartPlay();
}

void Behavior::Play(const Stecchino::AccelStatus accel_status) {
    Log.trace(F("Behavior::Play\n"));

    unsigned long elapsed_time = millis() - start_time_;

    if (elapsed_time > MAX_PLAY_MS) {
        SetState(Stecchino::State::kSleepTransition);
        return;
    }

    if (accel_status == Stecchino::AccelStatus::kFallen) {
        SetState(Stecchino::State::kGameOverTransition);
        return;
    }

    if (elapsed_time > record_time_) {
        record_time_ = elapsed_time;
    }

    if (elapsed_time > previous_record_time_ && elapsed_time <= previous_record_time_ + 1000 &&
        previous_record_time_ > 0) {
        led_strip_->ShowWinner();
    }

    if (NUM_LEDS_PER_SECOND * static_cast<int>(elapsed_time / 1000) >= NUM_LEDS) {
        led_strip_->ShowWinner();
    } else {
        led_strip_->On(NUM_LEDS_PER_SECOND * static_cast<int>(elapsed_time / 1000),
                       NUM_LEDS_PER_SECOND * static_cast<int>(record_time_ / 1000));
    }
}

void Behavior::GameOverTransition(void) {
    Log.trace(F("Behavior::GameOverTransition\n"));

    if (millis() - start_time_ > MAX_GAME_OVER_TRANSITION_MS) {
        SetState(Stecchino::State::kIdle);
        return;
    }

    led_strip_->ShowPattern(LedStrip::Pattern::kGameOver);
}

void Behavior::SpiritLevel(const float angle_to_horizon, const Stecchino::Orientation orientation) {
    Log.trace(F("Behavior::SpiritLevel\n"));

    if (orientation == Stecchino::Orientation::kPosition_1 || orientation == Stecchino::Orientation::kPosition_2) {
        SetState(Stecchino::State::kIdle);
        return;
    }

    if (millis() - start_time_ > MAX_SPIRIT_LEVEL_MS) {
        SetState(Stecchino::State::kFakeSleep);
        return;
    }

    led_strip_->ShowSpiritLevel(angle_to_horizon);
}

void Behavior::FakeSleep(const float angle_to_horizon) {
    Log.trace(F("Behavior::FakeSleep\n"));

    if (millis() - start_time_ > MAX_FAKE_SLEEP_MS) {
        SetState(Stecchino::State::kSleepTransition);
        return;
    }

    if (abs(angle_to_horizon) > 15) {
        SetState(Stecchino::State::kIdle);
        return;
    }

    led_strip_->Off();
}

void Behavior::SleepTransition(void) {
    Log.trace(F("Behavior::SleepTransition\n"));

    if (millis() - start_time_ <= MAX_SLEEP_TRANSITION_MS) {
        led_strip_->ShowGoingToSleep();
        return;
    }

    // Go to sleep. This will continue when interrupted.
    Sleep();

    // position_->ClearSampleBuffer();

    // Restore MPU connection.
    // XXX Log.notice(F("Restarting MPU\n"));
    // XXX mpu_->Setup();

    SetState(Stecchino::State::kCheckBattery);
}
