#pragma once

#include <avr/sleep.h>

#include <Arduino.h>

#include "ArduinoLog.h"
#include "Configuration.h"

volatile bool interrupted = false;

void pinInterrupt(void) {
    if (interrupted) {
        Log.notice(F("Interrupted, disabling interrupt\n"));
        detachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT));
    }
}

void sleepNow(volatile bool & interrupted) {
    Log.trace(F("sleepNow()\n"));

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    interrupted = false;
    //attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), pinInterrupt, LOW);

    Log.trace(F("powering down\n"));

    // Put the device to sleep:
    digitalWrite(PIN_MOSFET_GATE, LOW);  // Turn LEDs off to indicate sleep.

    // Turn MPU off.
    // XXX digitalWrite(PIN_MPU_POWER, LOW);
    delay(100);  // XXX needed?

    Log.trace(F("sleeping\n"));

    Serial.flush();
    sleep_mode();

    // Upon waking up, sketch continues from this point.
    Log.trace(F("woke\n"));
    interrupted = true;
    sleep_disable();

    // Turn LEDs on to indicate awake.
    digitalWrite(PIN_MOSFET_GATE, HIGH);

    // Turn MPU on.
    // XXX digitalWrite(PIN_MPU_POWER, HIGH);

    delay(100);  // XXX needed?
}
