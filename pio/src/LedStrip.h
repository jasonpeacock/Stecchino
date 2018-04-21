#pragma once

#include <stdint.h>

#include <FastLED.h>

#include "Configuration.h"

class LedStrip {
 public:
  LedStrip();

  void Setup(void);

  void Update(void);

  void SetColorAt(uint8_t position, CRGB::HTMLColorCode);

  void Off(void);

 private:
  const uint8_t led_count_ = NUM_LEDS;

  uint8_t hue_ = 0;

  CRGB leds_[NUM_LEDS];
};
