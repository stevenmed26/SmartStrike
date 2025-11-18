#pragma once

#include "strikezone/types.hpp"

#include <optional>
#include <vector>

namespace sz {

struct BallState; // from ball_tracker.hpp

class DecisionEngine {
public:
    DecisionEngine() = default;

    // Computes if trajectory intersects the strike zone polygon.
    // Returns PitchEvent with isStrike=true if it crosses the zone,
    // otherwise a BALL event anchored to the last trajectory point.
    std::optional<PitchEvent> evaluatePitch(
        const std::vector<BallState>& trajectory,
        const StrikeZoneBounds& zone) const;
};

} // namespace sz

