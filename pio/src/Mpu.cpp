#include "Mpu.h"

#include <ArduinoLog.h>
#include <MPU6050.h>

#include "Configuration.h"

Mpu::Mpu(void){};

// Setup MPU.
bool Mpu::Setup(void) {
    Log.trace(F("Mpu::Setup\n"));

    pinMode(PIN_MPU_POWER, OUTPUT);
    digitalWrite(PIN_MPU_POWER, HIGH);

    // Wait for the MPU to turn on.
    // XXX can this be shorter?
    delay(500);

    Wire.begin();

    mpu_.initialize();

    if (!mpu_.testConnection()) {
        Log.error(F("MPU6050 connection failed\n"));
        return false;
    }

    Log.notice(F("MPU6050 connection successful\n"));

    // Set to active-low (1) to trigger the LOW interrupt signal when motion is detected
    // and wake via the interrupt pin which is set to HIGH.
    mpu_.setInterruptMode(true);

    mpu_.setIntMotionEnabled(true);

    // Only trigger if significant movement duration.
    mpu_.setMotionDetectionThreshold(2);
    mpu_.setMotionDetectionDuration(UINT8_MAX);

    // Enabling the high-pass filter makes the MPU sensitive to movement,
    // not just accel. Previously would only respond to tapping/knocking,
    // now it reacts to moving/being picked up.
    mpu_.setDHPFMode(MPU6050_DHPF_0P63);

    Log.verbose(F("Interrupt mode       : [%T]\n"), mpu_.getInterruptMode());
    Log.verbose(F("Interrupt drive      : [%T]\n"), mpu_.getInterruptDrive());
    Log.verbose(F("Interrupt latch      : [%T]\n"), mpu_.getInterruptLatch());
    Log.verbose(F("Interrupt latch clean: [%T]\n"), mpu_.getInterruptLatchClear());
    Log.verbose(F("Interrupt freefall   : [%T]\n"), mpu_.getIntFreefallEnabled());
    Log.verbose(F("Interrupt motion     : [%T]\n"), mpu_.getIntMotionEnabled());
    Log.verbose(F("Interrupt zero motion: [%T]\n"), mpu_.getIntZeroMotionEnabled());
    Log.verbose(F("Interrupt data ready : [%T]\n"), mpu_.getIntDataReadyEnabled());

    Log.verbose(F("Motion detection threshold: [%d]\n"), mpu_.getMotionDetectionThreshold());
    Log.verbose(F("Motion detection duration : [%d]\n"), mpu_.getMotionDetectionDuration());

    Log.verbose(F("DLPF Mode: [%d]\n"), mpu_.getDLPFMode());
    Log.verbose(F("DHPF Mode: [%d]\n"), mpu_.getDHPFMode());

    return true;
}

void Mpu::GetAccelMotion(int16_t * x_a, int16_t * y_a, int16_t * z_a) {
    Log.trace(F("Mpu::GetMotion\n"));

    // Not used, but needed for the `getMotion6()` API.
    int16_t x_g = 0;
    int16_t y_g = 0;
    int16_t z_g = 0;

    mpu_.getMotion6(x_a, y_a, z_a, &x_g, &y_g, &z_g);
}
