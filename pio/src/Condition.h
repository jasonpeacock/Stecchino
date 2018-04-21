#include <Arduino.h>

#include <MPU6050.h>

#include "ArduinoLog.h"
#include "BatteryLevel.h"
#include "Configuration.h"
#include "LedPatterns.h"
#include "Mpu.h"
#include "Position.h"
#include "SleepNow.h"
#include "SpiritLevel.h"

unsigned long current_time         = 0;
unsigned long elapsed_time         = 0;
unsigned long record_time          = 0;
unsigned long previous_record_time = 0;
float         angle_to_horizon     = 0;
bool          ready_for_change     = false;
unsigned long start_time           = 0;

enum class Condition {
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

// Display battery level at startup as the initial condition.
Condition condition = Condition::kCheckBattery;

const char* kConditionNames[static_cast<int>(Condition::kCount)] = {
  "CheckBattery",
  "FakeSleep",
  "GameOverTransition",
  "Idle",
  "Play",
  "SleepTransition",
  "SpiritLevel",
  "StartPlayTransition",
};

void checkCondition(CRGB leds[], MPU6050 mpu) {
  Log.trace(F("checkCondition(): start\n"));

  Log.notice(F("Condition: [%s]\n"), kConditionNames[static_cast<int>(condition)]);

  angle_to_horizon = checkPosition(mpu);

  switch (condition) {
    case Condition::kCount:
      break;

    case Condition::kCheckBattery:
      showBatteryLevel(leds);
      delay(2000);
      allOff(leds);

      condition = Condition::kIdle;
      break;

    case Condition::kIdle:
      if (BUTTON_1_ON && ready_for_change) {
        //delay(20); // debouncing
        ready_for_change = false;

        // Change light patterns when button is pressed.
        nextPattern();

        // Restart counter to enjoy new pattern longer.
        start_time = millis();
      }

      if (!BUTTON_1_ON) {
        ready_for_change = true;
      }

      if (millis() - start_time > IDLE_MS) {
        condition  = Condition::kFakeSleep;
        start_time = millis();
      }

      if (accel_status == AccelStatus::kStraight) {
        condition  = Condition::kStartPlayTransition;
        start_time = millis();
      }

      if (orientation == Position::kPosition_3) {
        condition  = Condition::kSpiritLevel;
        start_time = millis();
      }

      if (orientation == Position::kPosition_2) {
        condition  = Condition::kSleepTransition;
        start_time = millis();
      } else {
        led(leds, Pattern::kIdle);
      }

      break;

    case Condition::kStartPlayTransition:
      /*
      if (millis() - start_time > START_PLAY_TRANSITION_MS) {
        condition = Condition::kPlay;
        start_time=millis();
        previous_record_time = record_time;
      } else {
        led(leds, Pattern::kStartPlay);
      }
      */

      allOff(leds);
      start_time           = millis();
      previous_record_time = record_time;

      condition = Condition::kPlay;
      break;

    case Condition::kPlay:
      elapsed_time = (millis() - start_time) / 1000;

      if (elapsed_time > record_time) {
        record_time = elapsed_time;
      }

      if (elapsed_time > previous_record_time &&
          elapsed_time <= previous_record_time + 1 &&
          previous_record_time != 0) {
        led(leds, Pattern::kWahoo);
      }

      if (elapsed_time > MAX_PLAY_SECONDS) {
        condition  = Condition::kSleepTransition;
        start_time = millis();
      }

      if (NUM_LEDS_PER_SECONDS * int(elapsed_time) >= NUM_LEDS) {
        led(leds, Pattern::kWahoo);
      } else {
        ledsOn(leds, NUM_LEDS_PER_SECONDS * int(elapsed_time), NUM_LEDS_PER_SECONDS* int(record_time));
      }

      if (accel_status == AccelStatus::kFallen) {
        condition  = Condition::kGameOverTransition;
        start_time = millis();
      }

      break;

    case Condition::kGameOverTransition:
      if (millis() - start_time > GAME_OVER_TRANSITION_MS) {
        condition  = Condition::kIdle;
        start_time = millis();
      } else {
        led(leds, Pattern::kGameOver);
      }

      break;

    case Condition::kSpiritLevel:
      if (orientation == Position::kPosition_1 || orientation == Position::kPosition_2) {
        condition  = Condition::kIdle;
        start_time = millis();
      }

      if (millis() - start_time > MAX_SPIRIT_LEVEL_MS) {
        condition  = Condition::kFakeSleep;
        start_time = millis();
      }

      spiritLevel(leds, angle_to_horizon);

      break;

    case Condition::kFakeSleep:
      if (millis() - start_time > FAKE_SLEEP_MS) {
        condition  = Condition::kSleepTransition;
        start_time = millis();
      }

      if (abs(angle_to_horizon) > 15) {
        condition  = Condition::kIdle;
        start_time = millis();
      }

      if (BUTTON_1_ON) {
        condition        = Condition::kIdle;
        ready_for_change = false;
        start_time       = millis();
      } else {
        led(leds, Pattern::kOff);
      }

      break;

    case Condition::kSleepTransition:
      if (millis() - start_time > SLEEP_TRANSITION_MS) {
        // Go to sleep. This will continue when interrupted.
        sleepNow(interrupted);

        // Clear running median buffer.
        a_forward_rolling_sample.clear();
        a_sideway_rolling_sample.clear();
        a_vertical_rolling_sample.clear();

        // Restore MPU connection.
        Log.notice(F("Restarting MPU\n"));
        setupMpu(mpu);

        condition  = Condition::kCheckBattery;
        start_time = millis();
      } else {
        led(leds, Pattern::kGoingToSleep);
      }

      break;
  }

  Log.trace(F("checkCondition(): end\n"));
}
