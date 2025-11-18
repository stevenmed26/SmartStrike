#pragma once

#include "strikezone/types.hpp"

namespace sz {

class BatterDetector {
public:
    BatterDetector() = default;

    // Returns approximate top and bottom y-coordinates of the batter’s silhouette.
    // Returns true on success, false if no batter is detected.
    bool detectBatter(const Frame& frame, double& topY, double& bottomY) const;
};

} // namespace sz

