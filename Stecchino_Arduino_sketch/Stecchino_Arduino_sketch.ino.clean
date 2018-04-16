#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>

#include "FastLED.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "RunningMedian.h"
#include "Wire.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Pins
#define DATA_PIN      5 // orig: 10
#define MOSFET_GATE   4 // orig: 11
#define MPU_POWER_PIN 6 // orig: 9
#define PUSH_B1       3 // orig: 2

#define BUTTON_1_ON (!digitalRead(PUSH_B1))

// LEDs
#define NUM_LEDS           9  // How many leds in your strip? (orig: 72)
#define HIGH_BRIGHTNESS    50 // Set LEDS brightness
#define LOW_BRIGHTNESS     10 // Set LEDS brightness

// Power Management
#define IDLE_MS             20000 // how many milliseconds at idle before moving to Fake_sleep?
#define FAKE_SLEEP_MS       60000 // how many milliseconds at Fake_sleep before moving to Sleep?
#define MAX_SPIRIT_LEVEL_MS 20000 // how many milliseconds at Fake_sleep before moving to Sleep?
#define MAX_PLAY_SECONDS    60    // how many seconds vertical before moving to Sleep?
                                  // (in case Stecchino is forgotten vertical)

#define NUM_LEDS_PER_SECONDS 2 // how many LEDs are turned on every second when playing?

// LED Animation
#define FRAMES_PER_SECOND         120
#define WAKE_UP_TRANSITION_MS    1000 // duration of animation when system returns from sleep
#define START_PLAY_TRANSITION_MS  500 // duration of animation when game starts
#define GAME_OVER_TRANSITION_MS  1000 // duration of game over animation
#define SLEEP_TRANSITION_MS      2000 // duration of animation to sleep

#define LOW_VCC  2700 // lower vcc value when checking battery level
#define HIGH_VCC 3350 // higher vcc value when checking battery level

#define ACCELEROMETER_ORIENTATION 2 // 0, 1 or 2 to set the angle of the joystick

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

String accel_status = String("unknown");
RunningMedian a_forward_rolling_sample = RunningMedian(5);
RunningMedian a_sideway_rolling_sample = RunningMedian(5);
RunningMedian a_vertical_rolling_sample = RunningMedian(5);

int a_forward_offset = 0;
int a_sideway_offset = 0;
int a_vertical_offset = 0;

// Used to detect position of buttons relative to Stecchino and user
enum POSITION_STECCHINO {
  NONE,
  POSITION_1,
  POSITION_2,
  POSITION_3,
  POSITION_4,
  POSITION_5,
  POSITION_6,
  COUNT
};

// POSITION_1: Stecchino V3/V4 horizontal with buttons up (idle)
// POSITION_2: Stecchino V3/V4 horizontal with buttons down (force sleep)
// POSITION_3: Stecchino V3/V4 horizontal with long edge down (spirit level)
// POSITION_4: Stecchino V3/V4 horizontal with short edge down (opposite to spirit level)
// POSITION_5: Stecchino V3/V4 vertical with PCB up (normal game position = straight)
// POSITION_6: Stecchino V3/V4 vertical with PCB down (easy game position = straight)
const char * position_stecchino[COUNT] = {
  "None",
  "POSITION_1",
  "POSITION_2",
  "POSITION_3",
  "POSITION_4",
  "POSITION_5",
  "POSITION_6"
};


uint8_t orientation = NONE;

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
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV(hue, 255, 192);
}

// eight colored dots, weaving in and out of sync with each other
void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void bpm() {
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
}

// FastLED's built-in rainbow generator
void rainbow() {
  fill_rainbow(leds, NUM_LEDS, hue, 7);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[]) ();
SimplePatternList g_patterns = {
  confetti,
  sinelon,
  juggle,
  bpm,
  rainbow
};

void redGlitter() {
  frame_count += 1;
  if (frame_count % 4 == 1) { // Slow down frame rate
    for ( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(HUE_RED, 0, random8() < 60 ? random8() : random8(64));
    }
  }
}

void ledsOn(int count, int record) {
  for (int i = NUM_LEDS; i >= 0; i--) {
    if (i <= NUM_LEDS - count) {
      leds[i] = CRGB::Black;
    } else {
      leds[i] = CHSV(hue, 255, 255);
    }

    if (i == NUM_LEDS - record) {
      leds[i] = CRGB::Red;
    }
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    // slowly cycle the "base color" through the rainbow
    hue++;
  }
}

void LED(String pattern) {
  if (pattern == "going_to_sleep") {
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = NUM_LEDS; i >=0; i--) {
      leds[i]=CRGB::Blue;
    }
  }

  if (pattern == "idle"){
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    g_patterns[current_pattern_number]();

    //confetti();
    //rainbow();
  }

  if (pattern == "start_play"){
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = NUM_LEDS; i >=0; i--) {
      leds[i] = CRGB::Green;
    }
  }

  if (pattern == "wahoo"){
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    redGlitter();
  }

  if (pattern == "spirit_level") {
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    sinelon();
  }

  if (pattern == "game_over") {
    digitalWrite(MOSFET_GATE, HIGH);
    FastLED.setBrightness(LOW_BRIGHTNESS);
    for (int i = NUM_LEDS; i >=0; i--) {
      leds[i] = CRGB::Red;
    }
  }

  if (pattern == "off") {
    for (int i = NUM_LEDS; i >=0; i--) {
      //leds[i]=CRGB::Black;
      leds[i].nscale8(230);
    }
    digitalWrite(MOSFET_GATE, LOW);
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    // slowly cycle the "base color" through the rainbow
    hue++;
  }
}

void SPIRIT_LEVEL_LED(float angle) {
  digitalWrite(MOSFET_GATE, HIGH);
  FastLED.setBrightness(HIGH_BRIGHTNESS);
  int int_angle = int(angle);
  //int pos_led=map(int_angle,-90,90,1,NUM_LEDS);
  //int pos_led=map(int_angle,45,-45,1,NUM_LEDS);
  int pos_led = map(int_angle, -45, 45, 1, NUM_LEDS);
  int couleur_led = map(pos_led, 0, NUM_LEDS, 0, 255);
  for (int i = NUM_LEDS; i >=0; i--) {
    if (i == pos_led) {
      leds[i] = CHSV(couleur_led, 255, 255);
    } else {
      leds[i] = CRGB::Black;
    }
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    // slowly cycle the "base color" through the rainbow
    hue++;
  }
}

// bargraph showing battery level
void checkBatteryLed(int vcc) {
  digitalWrite(MOSFET_GATE, HIGH);
  FastLED.setBrightness(LOW_BRIGHTNESS);
  if (vcc < LOW_VCC) {
    vcc = LOW_VCC;
  }

  if (vcc > HIGH_VCC) {
    vcc = HIGH_VCC;
  }

  int pos_led = map(vcc, LOW_VCC, HIGH_VCC, 1, NUM_LEDS);

  Serial.println(vcc);
  Serial.println(LOW_VCC);
  Serial.println(HIGH_VCC);
  Serial.println(pos_led);

  for (int i = NUM_LEDS; i >=0; i--) {
    if (i <= pos_led){
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

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  // insert a delay to keep the framerate modest
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  // do some periodic updates
  EVERY_N_MILLISECONDS(20) {
    // slowly cycle the "base color" through the rainbow
    hue++;
  }
}

void nextPattern() {
  // add one to the current pattern number, and wrap around at the end
  current_pattern_number = (current_pattern_number + 1) % ARRAY_SIZE(g_patterns);
}

void allOff() {
  for (int i = NUM_LEDS; i >=0; i--) {
    leds[i]=CRGB::Black;
    delay(10);
    FastLED.show();
  }
}

enum {
  Check_Battery,
  Fake_Sleep,
  Game_Over_Transition,
  Idle,
  Magic_Wand,
  Play,
  Sleep_Transition,
  Spirit_Level,
  Start_Play_Transition,
  Wahoo,
  Wake_Up_Transition
} condition = Idle;

float checkAccel() {
  // Reads acceleration from MPU6050 to evaluate current condition.
  // Tunables:
  // Output values: accel_status=fallen or straight
  //                orientation=POSITION_1 to POSITION_6
  //                angle_2_horizon
  // Get accelerometer readings
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float angle_2_horizon=0;

  // Offset accel readings
  //int a_forward_offset = -2;
  //int a_sideway_offset = 2;
  //int a_vertical_offset = 1;

  // Convert to expected orientation - includes unit conversion to "cents of g" for MPU range set to 2g
  int a_forward = (ACCELEROMETER_ORIENTATION == 0 ? ax : (ACCELEROMETER_ORIENTATION == 1 ? ay : az)) / 164;
  int a_sideway = (ACCELEROMETER_ORIENTATION == 0 ? ay : (ACCELEROMETER_ORIENTATION == 1 ? az : ax)) / 164;
  int a_vertical = (ACCELEROMETER_ORIENTATION == 0 ? az : (ACCELEROMETER_ORIENTATION == 1 ? ax : ay)) / 164;

  // Update rolling average for smoothing
  a_forward_rolling_sample.add(a_forward);
  a_sideway_rolling_sample.add(a_sideway);
  a_vertical_rolling_sample.add(a_vertical);

  // Get median
  int a_forward_rolling_sample_median = a_forward_rolling_sample.getMedian() - a_forward_offset;
  int a_sideway_rolling_sample_median = a_sideway_rolling_sample.getMedian() - a_sideway_offset;
  int a_vertical_rolling_sample_median = a_vertical_rolling_sample.getMedian() - a_vertical_offset;

  // Evaluate current condition based on smoothed accelarations
  accel_status = "unknown";
  if (abs(a_sideway_rolling_sample_median) > abs(a_vertical_rolling_sample_median) ||
      abs(a_forward_rolling_sample_median) > abs(a_vertical_rolling_sample_median)) {
    accel_status="fallen";
  }

  if (abs(a_sideway_rolling_sample_median) < abs(a_vertical_rolling_sample_median) &&
      abs(a_forward_rolling_sample_median) < abs(a_vertical_rolling_sample_median)) {
    accel_status="straight";
  }
  //else {accel_status="unknown";}

  if (a_vertical_rolling_sample_median >= 80 &&
      abs(a_forward_rolling_sample_median) <= 25 &&
      abs(a_sideway_rolling_sample_median) <= 25 &&
      orientation != POSITION_6) {
    // coté 1 en haut
    orientation = POSITION_6;
  } else if (a_forward_rolling_sample_median >= 80 &&
      abs(a_vertical_rolling_sample_median) <= 25 &&
      abs(a_sideway_rolling_sample_median) <= 25 &&
      orientation != POSITION_2) {
    // coté 2 en haut
    orientation = POSITION_2;
  } else if (a_vertical_rolling_sample_median <= -80 &&
      abs(a_forward_rolling_sample_median) <= 25 &&
      abs(a_sideway_rolling_sample_median) <= 25 &&
      orientation != POSITION_5) {
    // coté 3 en haut
    orientation = POSITION_5;
  } else if (a_forward_rolling_sample_median <= -80 &&
      abs(a_vertical_rolling_sample_median) <= 25 &&
      abs(a_sideway_rolling_sample_median) <= 25 &&
      orientation != POSITION_1) {
    // coté 4 en haut
    orientation = POSITION_1;
  } else if (a_sideway_rolling_sample_median >= 80 &&
      abs(a_vertical_rolling_sample_median) <= 25 &&
      abs(a_forward_rolling_sample_median) <= 25 &&
      orientation != POSITION_3) {
    // coté LEDs en haut
    orientation = POSITION_3;
  } else if (a_sideway_rolling_sample_median <= -80 &&
      abs(a_vertical_rolling_sample_median) <= 25 &&
      abs(a_forward_rolling_sample_median) <= 25 &&
      orientation != POSITION_4) {
    // coté batteries en haut
    orientation = POSITION_4;
  }  else {
    // orientation = FACE_NONE;
  }

  angle_2_horizon = atan2(
      float(a_vertical_rolling_sample_median),
      float(max(abs(a_sideway_rolling_sample_median),
          abs(a_forward_rolling_sample_median)))) * 180 / PI;

  // for debugging
  Serial.print("a_forward:");
  Serial.print(a_forward_rolling_sample_median);
  Serial.print(" a_sideway:");
  Serial.print(a_sideway_rolling_sample_median);
  Serial.print(" a_vertical:");
  Serial.print(a_vertical_rolling_sample_median);
  Serial.print(" - ");
  Serial.print(position_stecchino[orientation]);
  Serial.print(" - ");
  Serial.print(accel_status);
  Serial.print(" - ");
  Serial.println(angle_2_horizon);

  return angle_2_horizon;
}

void pinInterrupt(void) {
  detachInterrupt(0);
}

void sleepNow(void) {
  // Set pin 2 as interrupt and attach handler:
  attachInterrupt(0, pinInterrupt, LOW);
  delay(100);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Set sleep enable (SE) bit:
  sleep_enable();

  // Put the device to sleep:
  digitalWrite(MOSFET_GATE, LOW);   // Turn LEDs off to indicate sleep.
  digitalWrite(MPU_POWER_PIN, LOW); // Turn MPU off.
  sleep_mode();

  // Upon waking up, sketch continues from this point.
  sleep_disable();
  digitalWrite(MOSFET_GATE, HIGH);   // Turn LEDs on to indicate awake.
  digitalWrite(MPU_POWER_PIN, HIGH); // Turn MPU on.
  delay(100);

  // clear running median buffer
  a_forward_rolling_sample.clear();
  a_sideway_rolling_sample.clear();
  a_vertical_rolling_sample.clear();

  // retore MPU connection
  Wire.begin();
  accelgyro.initialize();
  Serial.println("Restarting MPU... ");

  condition = Check_Battery;
  start_time=millis();
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle

  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

unsigned long current_time = 0;
unsigned long elapsed_time = 0;
unsigned long record_time = 0;
unsigned long previous_record_time = 0;
int i = 0;
int vcc = 0;
float angle_2_horizon = 0;
bool ready_for_change = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(PUSH_B1, INPUT);
  // Configure built-in pullup resitor for push button 1
  digitalWrite(PUSH_B1, HIGH);

  pinMode(MOSFET_GATE,OUTPUT);
  digitalWrite(MOSFET_GATE,HIGH);

  pinMode(MPU_POWER_PIN,OUTPUT);
  digitalWrite(MPU_POWER_PIN,HIGH);

  delay(500);

  // LEDs strip
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  // MPU
  Wire.begin();
  accelgyro.initialize();
  Serial.println("Starting MPU... ");

  //a_forward_offset=0;
  //a_sideway_offset=0;
  //a_vertical_offset=-100;

  //delay(2000);

  vcc = int(readVcc());
  Serial.print("VCC=");
  Serial.print(vcc);
  Serial.println("mV");
  checkBatteryLed(vcc);
  delay(2000);
  allOff();

  start_time = millis();
}

void loop() {
  angle_2_horizon = checkAccel();

  Serial.println(condition);

  switch (condition) {
    case Check_Battery:
      vcc = int(readVcc());
      Serial.print("VCC=");
      Serial.print(vcc);
      Serial.println("mV");
      checkBatteryLed(vcc);
      delay(2000);
      allOff();
      condition = Idle;
      break;

    case Wake_Up_Transition:
      break;

    case Idle:
      if (BUTTON_1_ON && ready_for_change) {
        //delay(20); // debouncing
        ready_for_change = false;
        nextPattern();  // change light patterns when button is pressed
        start_time = millis();  // restart counter to enjoy new pattern longer
      }
      if (!BUTTON_1_ON) {
        ready_for_change = true;
      }
      if (millis() - start_time > IDLE_MS) {
        condition = Fake_Sleep;
        start_time = millis();
      }
      if (accel_status == "straight") {
        condition = Start_Play_Transition;
        start_time = millis();
      }
      if (orientation == POSITION_3) {
        condition = Spirit_Level;
        start_time = millis();
      }
      if (orientation == POSITION_2) {
        condition = Sleep_Transition;
        start_time = millis();
      } else {
        LED("idle");
      }
      break;

    case Start_Play_Transition:
      /*
      if (millis() - start_time > START_PLAY_TRANSITION_MS) {
        condition = Play;
        start_time=millis();
        previous_record_time = record_time;
      } else {
        LED("start_play");
      }
      */

      allOff();
      condition = Play;
      start_time = millis();
      previous_record_time = record_time;
      break;

    case Play:
      elapsed_time = (millis() - start_time) / 1000;
      if (elapsed_time > record_time) {
        record_time = elapsed_time;
      }
      if (elapsed_time > previous_record_time &&
          elapsed_time <= previous_record_time + 1 &&
          previous_record_time != 0) {
        LED("wahoo");
      }
      if (elapsed_time > MAX_PLAY_SECONDS) {
        condition = Sleep_Transition;
        start_time = millis();
      }
      if (NUM_LEDS_PER_SECONDS * int(elapsed_time) >= NUM_LEDS) {
        LED("wahoo");
      } else {
        ledsOn(NUM_LEDS_PER_SECONDS * int(elapsed_time), NUM_LEDS_PER_SECONDS * int(record_time));
      }
      if (accel_status == "fallen") {
        condition = Game_Over_Transition;
        start_time=millis();
      }
      break;

    case Wahoo:
      break;

    case Game_Over_Transition:
      if (millis() - start_time > GAME_OVER_TRANSITION_MS) {
        condition = Idle;
        start_time = millis();
      } else {
        LED("game_over");
      }
      break;

    case Spirit_Level:
      if (orientation == POSITION_1 || orientation == POSITION_2) {
        condition = Idle;
        start_time = millis();
      }
      if (millis() - start_time > MAX_SPIRIT_LEVEL_MS) {
        condition = Fake_Sleep;
        start_time=millis();
      }
      SPIRIT_LEVEL_LED(angle_2_horizon);

      Serial.print("Angle to horizon:");
      Serial.println(angle_2_horizon);

      break;

    case Magic_Wand:
      break;

    case Fake_Sleep:
      if (millis() - start_time > FAKE_SLEEP_MS) {
        condition = Sleep_Transition;
        start_time = millis();
      }
      if (abs(angle_2_horizon) > 15) {
        condition = Idle;
        start_time=millis();
      }
      if (BUTTON_1_ON) {
        condition = Idle;
        ready_for_change = false;
        start_time=millis();
      } else {
        LED("off");
      }
      break;

    case Sleep_Transition:
      if (millis() - start_time > SLEEP_TRANSITION_MS) {
        sleepNow();
      } else {
        LED("going_to_sleep");
      }
      break;
  }

  FastLED.show();
}

