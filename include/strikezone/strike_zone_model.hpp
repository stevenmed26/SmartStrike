#pragma once

#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <vector>

namespace sz {

struct StrikeZoneConfig; // from config.hpp

class StrikeZoneModel {
public:
    explicit StrikeZoneModel(const StrikeZoneConfig& cfg);

    // Compute strike zone polygon based on plate and batter info (all in image coords)
    StrikeZoneBounds computeZone(const std::vector<cv::Point2f>& platePoly,
                                 double batterTopY,
                                 double batterBottomY) const;

private:
    StrikeZoneConfig cfg_;
};

} // namespace sz

