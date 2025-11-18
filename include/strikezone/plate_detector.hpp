#pragma once

#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <vector>

namespace sz {

class PlateDetector {
public:
    PlateDetector() = default;

    // Returns polygon representing plate footprint in image coordinates
    std::vector<cv::Point2f> detectPlate(const Frame& frame) const;
};

} // namespace sz

