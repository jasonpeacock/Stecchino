// Uncomment to compile out all logging code, will significantly reduce the code size,
// must come before the `ArduinoLog.h` include.
//#define DISABLE_LOGGING

// Third-party
#include <ArduinoLog.h>
#include <FastLED.h>
#include <MPU6050.h>

// Local
#include "Condition.h"
#include "Configuration.h"
#include "Mpu.h"

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

  start_time = millis();

  Log.trace(F("setup(): end\n"));
}

void loop() {
  Log.trace(F("loop(): start\n"));

  checkCondition(leds, mpu);

  // Slowly cycle the "base color" through the rainbow.
  EVERY_N_MILLISECONDS(20) { hue++; }

  FastLED.show();

  Log.trace(F("loop(): end\n"));
}
