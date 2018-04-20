#pragma once

#include "MPU6050.h"

MPU6050 mpu;

// Setup MPU.
void setupMpu(MPU6050& mpu) {
  Log.trace(F("setupMpu(): start\n"));

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Log.notice(F("MPU6050 connection failed\n"));
    return;
  }

  Log.notice(F("MPU6050 connection successful\n"));

  // Set to active-low (1) to trigger the LOW interrupt signal when motion is detected
  // and wake via the interrupt pin which is set to HIGH.
  mpu.setInterruptMode(true);

  mpu.setIntMotionEnabled(true);
  mpu.setMotionDetectionThreshold(2);

  // Only trigger if significant movement duration.
  mpu.setMotionDetectionDuration(UINT8_MAX);

  // Enabling the high-pass filter makes the MPU sensitive to movement,
  // not just accel. Previously would only respond to tapping/knocking,
  // now it reacts to moving/being picked up.
  mpu.setDHPFMode(MPU6050_DHPF_0P63);

  Log.verbose(F("Interrupt mode       : [%T]\n"), mpu.getInterruptMode());
  Log.verbose(F("Interrupt drive      : [%T]\n"), mpu.getInterruptDrive());
  Log.verbose(F("Interrupt latch      : [%T]\n"), mpu.getInterruptLatch());
  Log.verbose(F("Interrupt latch clean: [%T]\n"), mpu.getInterruptLatchClear());
  Log.verbose(F("Interrupt freefall   : [%T]\n"), mpu.getIntFreefallEnabled());
  Log.verbose(F("Interrupt motion     : [%T]\n"), mpu.getIntMotionEnabled());
  Log.verbose(F("Interrupt zero motion: [%T]\n"), mpu.getIntZeroMotionEnabled());
  Log.verbose(F("Interrupt data ready : [%T]\n"), mpu.getIntDataReadyEnabled());

  Log.verbose(F("Motion detection threshold: [%d]\n"), mpu.getMotionDetectionThreshold());
  Log.verbose(F("Motion detection duration : [%d]\n"), mpu.getMotionDetectionDuration());

  Log.verbose(F("DLPF Mode: [%d]\n"), mpu.getDLPFMode());
  Log.verbose(F("DHPF Mode: [%d]\n"), mpu.getDHPFMode());

  Log.trace(F("setupMpu(): end\n"));
}
