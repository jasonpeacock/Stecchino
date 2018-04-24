#pragma once

#include <stdint.h>

#include <FastLED.h>

#include "Configuration.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

class LedStrip {
  public:
    enum class Pattern {
        kGameOver,
        kSpiritLevel,
    };

    LedStrip();

    void Setup(void);

    void Update(void);

    void Off(void);

    void On(const int count, const int record);

    void ShowBatteryLevel(const int millivolts);

    void ShowSpiritLevel(const float angle);

    void ShowIdle();

    void ShowStartPlay();

    void ShowWinner();

    void ShowGoingToSleep();

    void ShowPattern(const LedStrip::Pattern pattern);

  private:
    // Increment by 1 for each Frame of Transition, New/Changed connection(s) pattern.
    uint8_t frame_count_;

    uint8_t hue_;

    const uint8_t led_count_;

    CRGB leds_[NUM_LEDS];

    void ConfettiPattern(void);

    void CylonPattern(void);
};
