#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "ArduinoLog.h"
#include "FastLED.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "RunningMedian.h"
#include "Wire.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Pins
#define PIN_LED_DATA 5     // orig: 10
#define PIN_MOSFET_GATE 4  // orig: 11
#define PIN_MPU_POWER 6    // orig: 9
#define PIN_BUTTON_1 3     // orig: 2
#define PIN_INTERRUPT 8    // orig: 3

#define BUTTON_1_ON (!digitalRead(PIN_BUTTON_1))

// LEDs
#define NUM_LEDS 72         // How many leds in your strip?
#define HIGH_BRIGHTNESS 50  // Set LEDS brightness
#define LOW_BRIGHTNESS 10   // Set LEDS brightness

// Power Management
#define IDLE_MS 20000              // how many milliseconds at idle before moving to Fake_sleep?
#define FAKE_SLEEP_MS 60000        // how many milliseconds at Fake_sleep before moving to Sleep?
#define MAX_SPIRIT_LEVEL_MS 20000  // how many milliseconds at Fake_sleep before moving to Sleep?
#define MAX_PLAY_SECONDS 60        // how many seconds vertical before moving to Sleep?
                                   // (in case Stecchino is forgotten vertical)

#define NUM_LEDS_PER_SECONDS 2  // how many LEDs are turned on every second when playing?

// LED Animation
#define FRAMES_PER_SECOND 120
#define WAKE_UP_TRANSITION_MS 1000    // duration of animation when system returns from sleep
#define START_PLAY_TRANSITION_MS 500  // duration of animation when game starts
#define GAME_OVER_TRANSITION_MS 1000  // duration of game over animation
#define SLEEP_TRANSITION_MS 2000      // duration of animation to sleep

#define LOW_VCC 2700   // lower vcc value when checking battery level
#define HIGH_VCC 3350  // higher vcc value when checking battery level

#define ACCELEROMETER_ORIENTATION 2  // 0, 1 or 2 to set the angle of the joystick

// Rotating "base color" used by many of the patterns.
uint8_t hue = 0;

// Index number of which pattern is current.
uint8_t current_pattern_number = 0;

// Increment by 1 for each Frame of Transition, New/Changed connection(s) pattern.
uint8_t frame_count = 0;

MPU6050 accelgyro;

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
const char* position_stecchino[static_cast<int>(Position::kCount)] = {
  "None",
  "POSITION_1",
  "POSITION_2",
  "POSITION_3",
  "POSITION_4",
  "POSITION_5",
  "POSITION_6",
};

unsigned long start_time = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];

// random colored speckles that blink in and fade smoothly
void confetti() {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(hue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void sinelon() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS);
  leds[pos] += CHSV(hue, 255, 192);
}

// eight colored dots, weaving in and out of sync with each other
void juggle() {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void bpm() {
  uint8_t       BeatsPerMinute = 62;
  CRGBPalette16 palette        = PartyColors_p;
  uint8_t       beat           = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; ++i) {  // 9948
    leds[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
}

// FastLED's built-in rainbow generator
void rainbow() {
  fill_rainbow(leds, NUM_LEDS, hue, 7);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList patterns = {
  confetti,
  sinelon,
  juggle,
  bpm,
  rainbow
};

void redGlitter() {
  frame_count += 1;
  if (frame_count % 4 == 1) {  // Slow down frame rate
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i] = CHSV(HUE_RED, 0, random8() < 60 ? random8() : random8(64));
    }
  }
}

void ledsOn(int count, int record) {
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (i <= NUM_LEDS - count) {
      leds[i] = CRGB::Black;
    } else {
      leds[i] = CHSV(hue, 255, 255);
    }

    if (i == NUM_LEDS - record) {
      leds[i] = CRGB::Red;
    }
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS(20) { hue++; }
}

enum class Pattern {
  kGameOver,
  kGoingToSleep,
  kIdle,
  kOff,
  kSpiritLevel,
  kStartPlay,
  kWahoo,
};

void led(Pattern pattern) {
  Log.trace(F("led(): start\n"));

  if (pattern == Pattern::kGoingToSleep) {
    Log.verbose(F("Pattern: GOING_TO_SLEEP\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i] = CRGB::Blue;
    }
  }

  if (pattern == Pattern::kIdle) {
    Log.verbose(F("Pattern: IDLE\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    patterns[current_pattern_number]();

    //confetti();
    //rainbow();
  }

  if (pattern == Pattern::kStartPlay) {
    Log.verbose(F("Pattern: START_PLAY\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i] = CRGB::Green;
    }
  }

  if (pattern == Pattern::kWahoo) {
    Log.verbose(F("Pattern: WAHOO\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    redGlitter();
  }

  if (pattern == Pattern::kSpiritLevel) {
    Log.verbose(F("Pattern: SPIRIT_LEVEL\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    sinelon();
  }

  if (pattern == Pattern::kGameOver) {
    Log.verbose(F("Pattern: GAME_OVER\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i] = CRGB::Red;
    }
  }

  if (pattern == Pattern::kOff) {
    Log.verbose(F("Pattern: OFF\n"));

    for (int i = 0; i < NUM_LEDS; ++i) {
      //leds[i]=CRGB::Black;
      leds[i].nscale8(230);
    }
    digitalWrite(PIN_MOSFET_GATE, LOW);
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // slowly cycle the "base color" through the rainbow
  EVERY_N_MILLISECONDS(20) { hue++; }

  Log.trace(F("led(): end\n"));
}

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
void showBatteryLevel(int vcc) {
  Log.trace(F("showBatteryLevel(): start\n"));

  Log.notice("VCC     : %dmV\n", vcc);
  Log.notice("LOW_VCC : %dmV\n", LOW_VCC);
  Log.notice("HIGH_VCC: %dmV\n", HIGH_VCC);

  digitalWrite(PIN_MOSFET_GATE, HIGH);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  if (vcc < LOW_VCC) {
    vcc = LOW_VCC;
  } else if (vcc > HIGH_VCC) {
    vcc = HIGH_VCC;
  }

  int pos_led = map(vcc, LOW_VCC, HIGH_VCC, 1, NUM_LEDS);
  Log.verbose("Showing battery level at LED Position: %d\n", pos_led);

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

void nextPattern() {
  // add one to the current pattern number, and wrap around at the end
  current_pattern_number = (current_pattern_number + 1) % ARRAY_SIZE(patterns);
}

void allOff() {
  Log.trace(F("allOff(): start\n"));

  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::Black;
    delay(10);
    FastLED.show();
  }

  Log.trace(F("allOff(): end\n"));
}

enum class Condition {
  kCheckBattery,
  kFakeSleep,
  kGameOverTransition,
  kIdle,
  kPlay,
  kSleepTransition,
  kSpiritLevel,
  kStartPlayTransition,
};

Condition condition = Condition::kIdle;

// Reads acceleration from MPU6050 to evaluate current condition.
// Tunables:
// Output values: accel_status=fallen or straight
//                orientation=POSITION_1 to POSITION_6
//                angle_to_horizon
float checkAcceleration() {
  Log.trace(F("checkAcceleration(): start\n"));

  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

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
      position_stecchino[static_cast<int>(orientation)],
      accel_status,
      angle_to_horizon);

  Log.trace(F("checkAcceleration(): end\n"));

  return angle_to_horizon;
}

void pinInterrupt(void) {
  Log.trace(F("pinInterrupt(): start\n"));

  detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));

  Log.trace(F("pinInterrupt(): end\n"));
}

void sleepNow(void) {
  Log.trace(F("sleepNow(): start\n"));

  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), pinInterrupt, LOW);
  delay(100);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Set sleep enable (SE) bit:
  sleep_enable();

  // Put the device to sleep:
  digitalWrite(PIN_MOSFET_GATE, LOW);  // Turn LEDs off to indicate sleep.
  digitalWrite(PIN_MPU_POWER, LOW);    // Turn MPU off.
  sleep_mode();

  // Upon waking up, sketch continues from this point.
  sleep_disable();
  digitalWrite(PIN_MOSFET_GATE, HIGH);  // Turn LEDs on to indicate awake.
  digitalWrite(PIN_MPU_POWER, HIGH);    // Turn MPU on.
  delay(100);

  // Clear running median buffer.
  a_forward_rolling_sample.clear();
  a_sideway_rolling_sample.clear();
  a_vertical_rolling_sample.clear();

  // Retore MPU connection.
  Wire.begin();
  accelgyro.initialize();
  Log.notice("Restarting MPU\n");

  condition  = Condition::kCheckBattery;
  start_time = millis();

  Log.trace(F("sleepNow(): end\n"));
}

long readVcc(void) {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);  // Wait for Vref to settle

  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;  // measuring

  uint8_t low  = ADCL;  // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result;  // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result;               // Vcc in millivolts
}

unsigned long current_time         = 0;
unsigned long elapsed_time         = 0;
unsigned long record_time          = 0;
unsigned long previous_record_time = 0;
int           i                    = 0;
int           vcc                  = 0;
float         angle_to_horizon     = 0;
bool          ready_for_change     = false;

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;

  Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
  Log.trace(F("setup(): start\n"));

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_INTERRUPT, INPUT);

  pinMode(PIN_MOSFET_GATE, OUTPUT);
  digitalWrite(PIN_MOSFET_GATE, HIGH);

  pinMode(PIN_MPU_POWER, OUTPUT);
  digitalWrite(PIN_MPU_POWER, HIGH);

  delay(500);

  // Setup LED strip.
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  // MPU
  Wire.begin();
  accelgyro.initialize();

  //a_forward_offset=0;
  //a_sideway_offset=0;
  //a_vertical_offset=-100;

  //delay(2000);

  vcc = static_cast<int>(readVcc());
  showBatteryLevel(vcc);

  delay(2000);

  allOff();

  start_time = millis();

  Log.trace(F("setup(): end\n"));
}

void loop() {
  Log.trace(F("loop(): start\n"));

  angle_to_horizon = checkAcceleration();

  //  Log.notice(condition);

  switch (condition) {
    case Condition::kCheckBattery:
      Log.verbose(F("Condition: CHECK_BATTERY\n"));

      vcc = int(readVcc());
      showBatteryLevel(vcc);
      delay(2000);
      allOff();

      condition = Condition::kIdle;
      break;

    case Condition::kIdle:
      Log.verbose(F("Condition: IDLE\n"));

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
        led(Pattern::kIdle);
      }

      break;

    case Condition::kStartPlayTransition:
      Log.verbose(F("Condition: START_PLAY_TRANSITION\n"));
      /*
      if (millis() - start_time > START_PLAY_TRANSITION_MS) {
        condition = Condition::kPlay;
        start_time=millis();
        previous_record_time = record_time;
      } else {
        led(START_PLAY);
      }
      */

      allOff();
      start_time           = millis();
      previous_record_time = record_time;

      condition = Condition::kPlay;
      break;

    case Condition::kPlay:
      Log.verbose(F("Condition: PLAY\n"));

      elapsed_time = (millis() - start_time) / 1000;

      if (elapsed_time > record_time) {
        record_time = elapsed_time;
      }

      if (elapsed_time > previous_record_time &&
          elapsed_time <= previous_record_time + 1 &&
          previous_record_time != 0) {
        led(Pattern::kWahoo);
      }

      if (elapsed_time > MAX_PLAY_SECONDS) {
        condition  = Condition::kSleepTransition;
        start_time = millis();
      }

      if (NUM_LEDS_PER_SECONDS * int(elapsed_time) >= NUM_LEDS) {
        led(Pattern::kWahoo);
      } else {
        ledsOn(NUM_LEDS_PER_SECONDS * int(elapsed_time), NUM_LEDS_PER_SECONDS* int(record_time));
      }

      if (accel_status == AccelStatus::kFallen) {
        condition  = Condition::kGameOverTransition;
        start_time = millis();
      }

      break;

    case Condition::kGameOverTransition:
      Log.verbose(F("Condition: GAME_OVER_TRANSITION\n"));

      if (millis() - start_time > GAME_OVER_TRANSITION_MS) {
        condition  = Condition::kIdle;
        start_time = millis();
      } else {
        led(Pattern::kGameOver);
      }

      break;

    case Condition::kSpiritLevel:
      Log.verbose(F("Condition: SPIRIT_LEVEL\n"));

      if (orientation == Position::kPosition_1 || orientation == Position::kPosition_2) {
        condition  = Condition::kIdle;
        start_time = millis();
      }

      if (millis() - start_time > MAX_SPIRIT_LEVEL_MS) {
        condition  = Condition::kFakeSleep;
        start_time = millis();
      }
      spiritLevelLed(angle_to_horizon);

      Log.notice("Angle to horizon: %F\n", angle_to_horizon);

      break;

    case Condition::kFakeSleep:
      Log.verbose(F("Condition: FAKE_SLEEP\n"));

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
        led(Pattern::kOff);
      }

      break;

    case Condition::kSleepTransition:
      Log.verbose(F("Condition: SLEEP_TRANSITION\n"));

      if (millis() - start_time > SLEEP_TRANSITION_MS) {
        sleepNow();
      } else {
        led(Pattern::kGoingToSleep);
      }

      break;
  }

  FastLED.show();

  Log.trace(F("loop(): end\n"));
}
