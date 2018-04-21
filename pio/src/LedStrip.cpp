#include "LedStrip.h"

#include <Arduino.h>
#include <ArduinoLog.h>
#include <FastLED.h>

#include "Configuration.h"

LedStrip::LedStrip() {}

// Setup LED strip.
void LedStrip::Setup(void) {
  pinMode(PIN_MOSFET_GATE, OUTPUT);
  digitalWrite(PIN_MOSFET_GATE, HIGH);

  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds_, static_cast<int>(led_count_));
  FastLED.setBrightness(LOW_BRIGHTNESS);
}

void LedStrip::Update(void) {
  Log.trace(F("LedStrip::Update(): start\n"));

  // Slowly cycle the "base color" through the rainbow.
  EVERY_N_MILLISECONDS(20) { ++hue_; }

  FastLED.show();

  Log.trace(F("LedStrip::Update(): end\n"));
}

void LedStrip::SetColorAt(uint8_t position, CRGB::HTMLColorCode color) {
  Log.trace(F("LedStrip::SetColorAt(): start\n"));

  leds_[position] = color;

  Log.trace(F("LedStrip::SetColorAt(): end\n"));
}

void LedStrip::Off(void) {
  Log.trace(F("LedStrip::Off(): start\n"));

  for (int i = 0; i < NUM_LEDS; ++i) {
    this->SetColorAt(i, CRGB::Black);
    delay(10);
    FastLED.show();
  }

  Log.trace(F("LedStrip::Off(): end\n"));
}
