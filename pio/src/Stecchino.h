#pragma once

namespace Stecchino {

    enum class AccelStatus {
        kFallen,
        kStraight,
        kUnknown,
    };

    // Used to detect position of buttons relative to Stecchino and user
    enum class Orientation {
        kNone,
        kPosition_1,
        kPosition_2,
        kPosition_3,
        kPosition_4,
        kPosition_5,
        kPosition_6,
    };

}