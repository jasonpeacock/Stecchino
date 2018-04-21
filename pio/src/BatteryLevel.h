#pragma once

#include "LedStrip.h"

class BatteryLevel {
 public:
  BatteryLevel(LedStrip* led_strip);

  void Show(void);

 private:
  LedStrip* led_strip_;

  long ReadVcc(void);
};
