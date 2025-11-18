#pragma once

#include <deque>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>

namespace sz {

struct BallDetection; // forward declaration from ball_detector.hpp

struct BallState {
    cv::Point2f position;
    double timestamp; // seconds
};

class BallTracker {
public:
    BallTracker() = default;

    // Add a ball detection at a given timestamp (seconds since start)
    void addDetection(const BallDetection& detection, double timestamp);

    // Return full trajectory if long enough to be meaningful
    std::optional<std::vector<BallState>> trajectory() const;

    // Whether a pitch is considered "in progress"
    bool isPitchInProgress() const;

private:
    std::deque<BallState> history_;
    std::size_t maxHistory_ = 60;
};

} // namespace sz

