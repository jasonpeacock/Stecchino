#pragma once

#include <stdint.h>

#include <FastLED.h>

#include "Configuration.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

class LedStrip {
  public:
    enum class Pattern {
        kGameOver,
        kGoingToSleep,
        kIdle,
        kOff,
        kSpiritLevel,
        kStartPlay,
        kWahoo,
    };

    LedStrip();

    void Setup(void);

    void Update(void);

    void Off(void);

    void On(const int count, const int record);

    void ShowBatteryLevel(const int millivolts);

    void ShowSpiritLevel(const float angle);

    void ShowPattern(const LedStrip::Pattern pattern);

    void NextPattern(void);

  private:
    // Index number of which pattern is current.
    uint8_t current_pattern_;

    // Increment by 1 for each Frame of Transition, New/Changed connection(s) pattern.
    uint8_t frame_count_;

    uint8_t hue_;

    const uint8_t led_count_;

    CRGB leds_[NUM_LEDS];

    // Winner!
    void RedGlitterPattern(void);

    // Entertainment
    void ConfettiPattern(void);

    // Entertainment
    void CylonPattern(void);

    // Entertainment
    void JugglePattern(void);

    // Entertainment
    void BpmPattern(void);

    // Entertainment
    void RainbowPattern(void);

    // Entertainment patterns that can be cycled through via button-press.
    typedef void (LedStrip::*PatternList[5])();

    PatternList patterns_ = {
        &LedStrip::ConfettiPattern,
        &LedStrip::CylonPattern,
        &LedStrip::JugglePattern,
        &LedStrip::BpmPattern,
        &LedStrip::RainbowPattern,
    };
};
