#include "BatteryLevel.h"

#include <Arduino.h>

#include "ArduinoLog.h"
#include "Configuration.h"
#include "LedStrip.h"

BatteryLevel::BatteryLevel(LedStrip* led_strip) : led_strip_(led_strip){};

// Display an bargraph of the battery level on the LED strip.
void BatteryLevel::Show(void) {
  Log.trace(F("BatteryLevel::Show(): start\n"));

  int vcc = static_cast<int>(this->ReadVcc());

  Log.notice(F("VCC     : %dmV\n"), vcc);
  Log.notice(F("LOW_VCC : %dmV\n"), LOW_VCC);
  Log.notice(F("HIGH_VCC: %dmV\n"), HIGH_VCC);

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
        led_strip_->SetColorAt(i, CRGB::Red);
      } else if (i > 5 && i <= 15) {
        led_strip_->SetColorAt(i, CRGB::Orange);
      } else {
        led_strip_->SetColorAt(i, CRGB::Green);
      }
    } else {
      led_strip_->SetColorAt(i, CRGB::Black);
    }
  }

  FastLED.show();
  FastLED.delay(1000 / FRAMES_PER_SECOND);

  Log.trace(F("BatteryLevel::Show(): end\n"));
}

long BatteryLevel::ReadVcc(void) {
  Log.trace(F("BatteryLevel::ReadVcc(): start\n"));

  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2);  // Wait for Vref to settle

  ADCSRA |= _BV(ADSC);  // Start conversion
  while (bit_is_set(ADCSRA, ADSC)) {
    // measuring
  }

  uint8_t low  = ADCL;  // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH;  // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result;  // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000

  Log.trace(F("BatteryLevel::ReadVcc(): end\n"));

  return result;  // Vcc in millivolts
}
