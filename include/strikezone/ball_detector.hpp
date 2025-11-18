#pragma once

#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <optional>

namespace sz {

struct BallDetection {
    cv::Point2f center;
    float radius;
};

class BallDetector {
public:
    BallDetector() = default;

    // Detect a single best ball candidate in the frame (if any)
    std::optional<BallDetection> detect(const Frame& frame) const;
};

} // namespace sz

