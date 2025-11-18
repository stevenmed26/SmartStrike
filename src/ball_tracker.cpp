#include "strikezone/ball_tracker.hpp"
#include "strikezone/ball_detector.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace sz {

namespace {

// These are internal tracking heuristics. If you later wire in YAML config,
// you can replace these with values loaded at runtime.

constexpr std::size_t kMinTrajectoryLength = 4;     // min samples to consider a real pitch
constexpr double      kMaxGapSeconds       = 0.40;  // if time gap > this, assume new pitch
constexpr float       kMaxJumpDistancePx   = 250.0f;// big jump ⇒ likely new pitch

constexpr bool  kSmoothingEnabled = true;
constexpr float kSmoothingFactor  = 0.75f;          // EMA factor for position smoothing

float distancePx(const cv::Point2f& a, const cv::Point2f& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

void BallTracker::addDetection(const BallDetection& detection, double timestamp)
{
    BallState newState;
    newState.timestamp = timestamp;

    const cv::Point2f rawPos = detection.center;

    if (!history_.empty()) {
        const BallState& last = history_.back();
        const double dt = timestamp - last.timestamp;
        const float jumpDist = distancePx(rawPos, last.position);

        // If there was a long temporal gap or a huge spatial jump,
        // treat this as the start of a new pitch.
        bool newPitch = false;
        if (dt > kMaxGapSeconds) {
            newPitch = true;
        }
        if (jumpDist > kMaxJumpDistancePx) {
            newPitch = true;
        }

        if (newPitch) {
            history_.clear();
            newState.position = rawPos;
        } else {
            if (kSmoothingEnabled) {
                // Exponential moving average smoothing of position
                const cv::Point2f smoothed(
                    kSmoothingFactor * rawPos.x + (1.0f - kSmoothingFactor) * last.position.x,
                    kSmoothingFactor * rawPos.y + (1.0f - kSmoothingFactor) * last.position.y
                );
                newState.position = smoothed;
            } else {
                newState.position = rawPos;
            }
        }
    } else {
        // First detection for this pitch
        newState.position = rawPos;
    }

    history_.push_back(newState);

    // Enforce history cap (maxHistory_ is defined in the header with a default value)
    if (history_.size() > maxHistory_) {
        history_.pop_front();
    }
}

std::optional<std::vector<BallState>> BallTracker::trajectory() const
{
    if (history_.size() < kMinTrajectoryLength) {
        // Not enough samples to form a meaningful trajectory
        return std::nullopt;
    }

    std::vector<BallState> traj;
    traj.reserve(history_.size());
    std::copy(history_.begin(), history_.end(), std::back_inserter(traj));
    return traj;
}

bool BallTracker::isPitchInProgress() const
{
    // A pitch is considered "in progress" once we have a minimal number of
    // consistent samples in the history.
    return history_.size() >= kMinTrajectoryLength;
}

} // namespace sz
