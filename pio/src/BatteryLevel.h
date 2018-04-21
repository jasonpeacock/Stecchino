#include <Arduino.h>

#include <FastLED.h>
#include <MPU6050.h>

#include "ArduinoLog.h"
#include "Configuration.h"
#include "ReadVcc.h"

// Bargraph showing battery level.
void showBatteryLevel(CRGB leds[]) {
  Log.trace(F("showBatteryLevel(): start\n"));

  int vcc = static_cast<int>(readVcc());

  Log.notice(F("VCC     : %dmV\n"), vcc);
  Log.notice(F("LOW_VCC : %dmV\n"), LOW_VCC);
  Log.notice(F("HIGH_VCC: %dmV\n"), HIGH_VCC);

  digitalWrite(PIN_MOSFET_GATE, HIGH);
  FastLED.setBrightness(LOW_BRIGHTNESS);

  if (vcc < LOW_VCC) {
    vcc = LOW_VCC;
  } else if (vcc > HIGH_VCC) {
    vcc = HIGH_VCC;
  }

  int pos_led = map(vcc, LOW_VCC, HIGH_VCC, 1, NUM_LEDS);
  Log.verbose(F("Showing battery level at LED Position: %d\n"), pos_led);

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

  Log.trace(F("showBatteryLevel(): end\n"));
}
