#include "LedStrip.h"

#include <Arduino.h>
#include <ArduinoLog.h>
#include <FastLED.h>

#include "Configuration.h"

LedStrip::LedStrip() : current_pattern_(0), frame_count_(0), hue_(0), led_count_(NUM_LEDS) {}

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

void LedStrip::Off(void) {
    Log.trace(F("LedStrip::Off(): start\n"));

    for (int i = 0; i < led_count_; ++i) {
        leds_[i] = CRGB::Black;
        delay(10);
        FastLED.show();
    }

    Log.trace(F("LedStrip::Off(): end\n"));
}

void LedStrip::On(const int count, const int record) {
    Log.trace(F("LedStrip::On(): start\n"));

    for (int i = 0; i < led_count_; ++i) {
        if (i <= led_count_ - count) {
            leds_[i] = CRGB::Black;
        } else {
            leds_[i] = CHSV(hue_, 255, 255);
        }

        if (i == led_count_ - record) {
            leds_[i] = CRGB::Red;
        }
    }

    FastLED.show();
    delay(1000 / FRAMES_PER_SECOND);

    Log.trace(F("LedStrip::On(): end\n"));
}

// random colored speckles that blink in and fade smoothly
void LedStrip::ConfettiPattern() {
    fadeToBlackBy(leds_, led_count_, 10);

    int pos = random16(led_count_);
    leds_[pos] += CHSV(hue_ + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void LedStrip::CylonPattern() {
    fadeToBlackBy(leds_, led_count_, 20);

    int pos = beatsin16(13, 0, led_count_);
    leds_[pos] += CHSV(hue_, 255, 192);
}

// eight colored dots, weaving in and out of sync with each other
void LedStrip::JugglePattern() {
    fadeToBlackBy(leds_, led_count_, 20);

    byte dot_hue = 0;
    for (int i = 0; i < 8; i++) {
        int pos = beatsin16(i + 7, 0, led_count_);
        leds_[pos] |= CHSV(dot_hue, 200, 255);

        dot_hue += 32;
    }
}

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void LedStrip::BpmPattern() {
    uint8_t       beats_per_minute = 62;
    CRGBPalette16 palette          = PartyColors_p;
    uint8_t       beat             = beatsin8(beats_per_minute, 64, 255);
    for (int i = 0; i < led_count_; ++i) {
        leds_[i] = ColorFromPalette(palette, hue_ + (i * 2), beat - hue_ + (i * 10));
    }
}

// FastLED's built-in rainbow generator
void LedStrip::RainbowPattern() {
    fill_rainbow(leds_, led_count_, hue_, 7);
}

void LedStrip::RedGlitterPattern() {
    frame_count_ += 1;

    if (frame_count_ % 4 == 1) {  // Slow down frame rate
        for (int i = 0; i < led_count_; ++i) {
            leds_[i] = CHSV(HUE_RED, 0, random8() < 60 ? random8() : random8(64));
        }
    }
}

void LedStrip::NextPattern() {
    // add one to the current pattern number, and wrap around at the end
    current_pattern_ = (current_pattern_ + 1) % ARRAY_SIZE(patterns_);
}

void LedStrip::ShowBatteryLevel(const int millivolts) {
    int pos_led = map(millivolts, LOW_VCC, HIGH_VCC, 1, led_count_);

    Log.verbose(F("Showing battery level at LED Position: %d\n"), pos_led);

    for (int i = 0; i < NUM_LEDS; ++i) {
        if (i <= pos_led) {
            if (i <= 5) {
                leds_[i] = CRGB::Red;
            } else if (i > 5 && i <= 15) {
                leds_[i] = CRGB::Orange;
            } else {
                leds_[i] = CRGB::Green;
            }
        } else {
            leds_[i] = CRGB::Black;
        }
    }

    FastLED.show();

    delay(2000);

    Off();
}

void LedStrip::ShowSpiritLevel(const float angle) {
    int int_angle = static_cast<int>(angle);

    int position = map(int_angle, -45, 45, 1, led_count_);
    int color    = map(position, 0, led_count_, 0, 255);

    for (int i = 0; i < led_count_; ++i) {
        if (i == position) {
            leds_[i] = CHSV(color, 255, 255);
        } else {
            leds_[i] = CRGB::Black;
        }
    }

    FastLED.show();

    delay(1000 / FRAMES_PER_SECOND);
}

void LedStrip::ShowPattern(const LedStrip::Pattern pattern) {
    Log.trace(F("LedStrip::ShowPattern(): start\n"));

    switch (pattern) {
        case LedStrip::Pattern::kGoingToSleep: {
            Log.verbose(F("Pattern: GOING_TO_SLEEP\n"));

            for (int i = 0; i < led_count_; ++i) {
                leds_[i] = CRGB::Blue;
            }
        } break;

        case LedStrip::Pattern::kIdle: {
            Log.verbose(F("Pattern: IDLE\n"));

            (this->*patterns_[current_pattern_])();
        } break;

        case LedStrip::Pattern::kStartPlay: {
            Log.verbose(F("Pattern: START_PLAY\n"));

            for (int i = 0; i < led_count_; ++i) {
                leds_[i] = CRGB::Green;
            }
        } break;

        case LedStrip::Pattern::kWahoo: {
            Log.verbose(F("Pattern: WAHOO\n"));

            RedGlitterPattern();
        } break;

        case LedStrip::Pattern::kSpiritLevel: {
            Log.verbose(F("Pattern: SPIRIT_LEVEL\n"));

            CylonPattern();
        } break;

        case LedStrip::Pattern::kGameOver: {
            Log.verbose(F("Pattern: GAME_OVER\n"));

            for (int i = 0; i < led_count_; ++i) {
                leds_[i] = CRGB::Red;
            }
        } break;

        case LedStrip::Pattern::kOff: {
            Log.verbose(F("Pattern: OFF\n"));

            for (int i = 0; i < led_count_; ++i) {
                // leds_[i]=CRGB::Black;
                leds_[i].nscale8(230);
            }
        } break;
    }

    FastLED.show();
    delay(1000 / FRAMES_PER_SECOND);

    Log.trace(F("LedStrip::ShowPattern(): end\n"));
}
