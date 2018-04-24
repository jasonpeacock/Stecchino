#include "BatteryLevel.h"

#include <Arduino.h>

#include "ArduinoLog.h"
#include "Configuration.h"

BatteryLevel::BatteryLevel(void){};

int BatteryLevel::GetMillivoltsForDisplay(void) const {
    Log.trace(F("BatteryLevel::GetMillivoltsForDisplay(): start\n"));

    int vcc = static_cast<int>(this->ReadVcc());

    Log.notice(F("VCC    : %dmV\n"), vcc);
    Log.notice(F("MIN_VCC: %dmV\n"), MIN_VCC_MV);
    Log.notice(F("MAX_VCC: %dmV\n"), MAX_VCC_MV);

    if (vcc < MIN_VCC_MV) {
        vcc = MIN_VCC_MV;
    } else if (vcc > MAX_VCC_MV) {
        vcc = MAX_VCC_MV;
    }

    Log.trace(F("BatteryLevel::MillivoltsForDisplay(): end\n"));

    return vcc;
}

long BatteryLevel::ReadVcc(void) const {
    Log.trace(F("BatteryLevel::ReadVcc(): start\n"));

    // Read 1.1V reference against AVcc. Set the reference to Vcc
    // and the measurement to the internal 1.1V reference.
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
#else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

    // Wait for Vref to settle.
    delay(2);

    // Start conversion.
    ADCSRA |= _BV(ADSC);

    while (bit_is_set(ADCSRA, ADSC)) {
        // Measuring.
    }

    // Must read ADCL first - it then locks ADCH.
    uint8_t low = ADCL;

    // Reading ADCH then unlocks both.
    uint8_t high = ADCH;

    long result = (high << 8) | low;

    // Calculate Vcc (in mV); 1125300 = 1.1 * 1023 * 1000
    result = 1125300L / result;

    Log.trace(F("BatteryLevel::ReadVcc(): end\n"));

    // Vcc in millivolts.
    return result;
}
