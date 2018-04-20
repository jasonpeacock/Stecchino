// System
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>

// Uncomment to compile out all logging code, will significantly reduce the code size,
// must come before the `ArduinoLog.h` include.
//#define DISABLE_LOGGING

// Third-party
#include <ArduinoLog.h>
#include <FastLED.h>
#include <I2Cdev.h>
#include <MPU6050.h>
#include <RunningMedian.h>
#include <Wire.h>

// Local
#include "Configuration.h"
#include "LedPatterns.h"
#include "Mpu.h"
#include "ReadVcc.h"

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

unsigned long start_time = 0;

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

Condition condition = Condition::kIdle;

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

void spiritLevelLed(float angle) {
  digitalWrite(PIN_MOSFET_GATE, HIGH);
  FastLED.setBrightness(HIGH_BRIGHTNESS);
  int int_angle = int(angle);
  //int pos_led=map(int_angle,-90,90,1,NUM_LEDS);
  //int pos_led=map(int_angle,45,-45,1,NUM_LEDS);
  int pos_led     = map(int_angle, -45, 45, 1, NUM_LEDS);
  int couleur_led = map(pos_led, 0, NUM_LEDS, 0, 255);
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (i == pos_led) {
      leds[i] = CHSV(couleur_led, 255, 255);
    } else {
      leds[i] = CRGB::Black;
    }
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS(20) { hue++; }
}

// bargraph showing battery level
void showBatteryLevel(void) {
  Log.trace(F("showBatteryLevel(): start\n"));

  int vcc = static_cast<int>(readVcc());

  Log.notice(F("VCC     : %dmV\n"), vcc);
  Log.notice(F("LOW_VCC : %dmV\n"), LOW_VCC);
  Log.notice(F("HIGH_VCC: %dmV\n"), HIGH_VCC);

  digitalWrite(PIN_MOSFET_GATE, HIGH);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  if (vcc < LOW_VCC) {
    vcc = LOW_VCC;
  } else if (vcc > HIGH_VCC) {
    vcc = HIGH_VCC;
  }

  int pos_led = map(vcc, LOW_VCC, HIGH_VCC, 1, NUM_LEDS);
  Log.verbose(F("Showing battery level at LED Position: %d\n"), pos_led);

  for (int i = 0; i < NUM_LEDS; ++i) {
    if (i <= pos_led) {
      if (i <= 5) {
        leds[i] = CRGB::Red;
      } else if (i > 5 && i <= 15) {
        leds[i] = CRGB::Orange;
      } else {
        leds[i] = CRGB::Green;
      }
    } else {
      leds[i] = CRGB::Black;
    }
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS(20) { hue++; }

  Log.trace(F("showBatteryLevel(): end\n"));
}

// Reads acceleration from MPU6050 to evaluate current condition.
// Tunables:
// Output values: accel_status=fallen or straight
//                orientation=POSITION_1 to POSITION_6
//                angle_to_horizon
float checkAcceleration() {
  Log.trace(F("checkAcceleration(): start\n"));

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

  Log.trace(F("checkAcceleration(): end\n"));

  return angle_to_horizon;
}

volatile bool interrupted = false;

void pinInterrupt(void) {
  Log.trace(F("pinInterrupt(): start\n"));

  if (interrupted) {
    Log.notice(F("Interrupted, disabling interrupt\n"));
    detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
  }

  Log.trace(F("pinInterrupt(): end\n"));
}

void sleepNow(void) {
  Log.trace(F("sleepNow(): start\n"));

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  interrupted = false;
  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), pinInterrupt, LOW);

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
  interrupted = true;
  sleep_disable();

  // Turn LEDs on to indicate awake.
  digitalWrite(PIN_MOSFET_GATE, HIGH);

  // Turn MPU on.
  // XXX digitalWrite(PIN_MPU_POWER, HIGH);

  delay(100);  // XXX needed?

  // Clear running median buffer.
  a_forward_rolling_sample.clear();
  a_sideway_rolling_sample.clear();
  a_vertical_rolling_sample.clear();

  // Restore MPU connection.
  Log.notice(F("Restarting MPU\n"));
  setupMpu(mpu);

  condition  = Condition::kCheckBattery;
  start_time = millis();

  Log.trace(F("sleepNow(): end\n"));
}

unsigned long current_time         = 0;
unsigned long elapsed_time         = 0;
unsigned long record_time          = 0;
unsigned long previous_record_time = 0;
int           i                    = 0;
int           vcc                  = 0;
float         angle_to_horizon     = 0;
bool          ready_for_change     = false;

void checkCondition() {
  Log.trace(F("checkCondition(): start\n"));

  Log.notice(F("Condition: [%s]\n"), kConditionNames[static_cast<int>(condition)]);

  angle_to_horizon = checkAcceleration();

  switch (condition) {
    case Condition::kCount:
      break;

    case Condition::kCheckBattery:
      showBatteryLevel();
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

      spiritLevelLed(angle_to_horizon);

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
        sleepNow();
      } else {
        led(leds, Pattern::kGoingToSleep);
      }

      break;
  }

  Log.trace(F("checkCondition(): end\n"));
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Wait for the Serial port to be ready.
    delay(100);
  }

  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.trace(F("setup(): start\n"));

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_INTERRUPT, INPUT_PULLUP);

  pinMode(PIN_MOSFET_GATE, OUTPUT);
  digitalWrite(PIN_MOSFET_GATE, HIGH);

  pinMode(PIN_MPU_POWER, OUTPUT);
  digitalWrite(PIN_MPU_POWER, HIGH);

  delay(500);

  // Setup LED strip.
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  setupMpu(mpu);

  showBatteryLevel();

  delay(2000);

  allOff(leds);

  start_time = millis();

  Log.trace(F("setup(): end\n"));
}

void loop() {
  Log.trace(F("loop(): start\n"));

  checkCondition();

  FastLED.show();

  Log.trace(F("loop(): end\n"));
}
