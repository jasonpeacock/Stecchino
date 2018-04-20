#pragma once

#include <Arduino.h>

#include "ArduinoLog.h"
#include "FastLED.h"

#include "Configuration.h"

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// Define the array of leds
CRGB leds[NUM_LEDS];

// Index number of which pattern is current.
uint8_t current_pattern_number = 0;

// Increment by 1 for each Frame of Transition, New/Changed connection(s) pattern.
uint8_t frame_count = 0;

// Rotating "base color" used by many of the patterns.
uint8_t hue = 0;

enum class Pattern {
  kGameOver,
  kGoingToSleep,
  kIdle,
  kOff,
  kSpiritLevel,
  kStartPlay,
  kWahoo,
};

// random colored speckles that blink in and fade smoothly
void confetti(CRGB leds[]) {
  fadeToBlackBy(leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV(hue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void sinelon(CRGB leds[]) {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = beatsin16(13, 0, NUM_LEDS);
  leds[pos] += CHSV(hue, 255, 192);
}

// eight colored dots, weaving in and out of sync with each other
void juggle(CRGB leds[]) {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
void bpm(CRGB leds[]) {
  uint8_t       BeatsPerMinute = 62;
  CRGBPalette16 palette        = PartyColors_p;
  uint8_t       beat           = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
}

// FastLED's built-in rainbow generator
void rainbow(CRGB leds[]) {
  fill_rainbow(leds, NUM_LEDS, hue, 7);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])(CRGB[]);
SimplePatternList patterns = {
  confetti,
  sinelon,
  juggle,
  bpm,
  rainbow,
};

void nextPattern() {
  // add one to the current pattern number, and wrap around at the end
  current_pattern_number = (current_pattern_number + 1) % ARRAY_SIZE(patterns);
}

void redGlitter(CRGB leds[]) {
  frame_count += 1;
  if (frame_count % 4 == 1) {  // Slow down frame rate
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i] = CHSV(HUE_RED, 0, random8() < 60 ? random8() : random8(64));
    }
  }
}

void ledsOn(CRGB leds[], int count, int record) {
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

void allOff(CRGB leds[]) {
  Log.trace(F("allOff(): start\n"));

  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::Black;
    delay(10);
    FastLED.show();
  }

  Log.trace(F("allOff(): end\n"));
}

void led(CRGB leds[], Pattern pattern) {
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
    patterns[current_pattern_number](leds);

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
    redGlitter(leds);
  }

  if (pattern == Pattern::kSpiritLevel) {
    Log.verbose(F("Pattern: SPIRIT_LEVEL\n"));

    digitalWrite(PIN_MOSFET_GATE, HIGH);
    FastLED.setBrightness(HIGH_BRIGHTNESS);
    sinelon(leds);
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
