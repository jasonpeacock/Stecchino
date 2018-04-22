#pragma once

#include "MPU6050.h"

class Mpu {
  public:
    Mpu(void);

    void Setup(void);

    void GetAccelMotion(int16_t * x_a, int16_t * y_a, int16_t * z_a);

  private:
    MPU6050 mpu_;
};
