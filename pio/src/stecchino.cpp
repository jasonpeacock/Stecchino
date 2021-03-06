// Uncomment to compile out all logging code, will significantly reduce the code size,
// must come before the `ArduinoLog.h` include.
//#define DISABLE_LOGGING

// System
#include <Arduino.h>

// Third-party
#include <ArduinoLog.h>

// Local
#include "behavior.h"
#include "configuration.h"
#include "ledStrip.h"
#include "mpu.h"
#include "position.h"
#include "stecchino.h"

Behavior *     behavior;
BatteryLevel * battery_level;
LedStrip *     led_strip;
Mpu *          mpu;
Position *     position;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        // Wait for the Serial port to be ready.
        delay(100);
    }

    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);
    Log.trace(F("setup(): start\n"));

    pinMode(PIN_INTERRUPT, INPUT_PULLUP);

    led_strip = new LedStrip();
    led_strip->Setup();

    battery_level = new BatteryLevel();

    mpu = new Mpu();
    if (!mpu->Setup()) {
        Log.fatal(F("Failed to setup the MPU\n"));
        Serial.flush();
        // TODO display error code pattern for MPU.
        exit(1);
    }

    position = new Position(mpu);
    position->Setup();

    behavior = new Behavior(led_strip, mpu, battery_level);
    behavior->Setup();

    Log.trace(F("setup(): end\n"));
}

Stecchino::State state;
Stecchino::State previous_state;

void loop() {
    Log.trace(F("loop(): start\n"));

    position->Update();

    float                  angle_to_horizon = position->GetAngleToHorizon();
    Stecchino::AccelStatus accel_status     = position->GetAccelStatus();
    Stecchino::Orientation orientation      = position->GetOrientation();

    behavior->Update(angle_to_horizon, accel_status, orientation);

    led_strip->Update();

    Log.trace(F("loop(): end\n"));
}
