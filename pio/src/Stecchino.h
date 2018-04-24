#pragma once

namespace Stecchino {

enum class State : int {
    kUnknown = 0,
    kCheckBattery,
    kFakeSleep,
    kGameOverTransition,
    kIdle,
    kPlay,
    kSleepTransition,
    kSpiritLevel,
    kStartPlayTransition,
};

enum class AccelStatus : int {
    kUnknown = 0,
    kFallen,
    kStraight,
};

// Used to detect position of buttons relative to Stecchino and user
enum class Orientation : int {
    kUnknown = 0,
    kPosition_1,
    kPosition_2,
    kPosition_3,
    kPosition_4,
    kPosition_5,
    kPosition_6,
};

}  // namespace Stecchino
