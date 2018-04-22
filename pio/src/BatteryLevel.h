#pragma once

class BatteryLevel {
  public:
    BatteryLevel(void);

    int GetMillivoltsForDisplay(void) const;

  private:
    long ReadVcc(void) const;
};
