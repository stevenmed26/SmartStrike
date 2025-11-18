#pragma once

#include "strikezone/types.hpp"

namespace sz {

struct BallDetection; // from ball_detector.hpp

class Visualizer {
public:
    Visualizer() = default;

    void drawPlate(Frame& frame, const std::vector<cv::Point2f>& platePoly) const;

    void drawStrikeZone(Frame& frame, const StrikeZoneBounds& zone) const;

    void drawBall(Frame& frame, const BallDetection& ball) const;

    void annotateDecision(Frame& frame, const PitchEvent& event) const;
};

} // namespace sz

