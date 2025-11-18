#pragma once

#include <opencv2/core.hpp>
#include <vector>

namespace sz {

// Convenience alias for image frames
using Frame = cv::Mat;

struct PitchEvent {
    bool isStrike;
    double crossingTime;      // seconds since pitch release
    cv::Point2f crossingPoint;// where ball crosses plane in image coords
};

struct StrikeZoneBounds {
    // Image-space polygon approximating the strike zone projection
    std::vector<cv::Point2f> polygon;
};

} // namespace sz

