#include <Arduino.h>

#include <FastLED.h>

#include "ArduinoLog.h"
#include "Configuration.h"
#include "LedPatterns.h"

void spiritLevel(CRGB leds[], float angle) {
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
}
