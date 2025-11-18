#include "strikezone/decision_engine.hpp"
#include "strikezone/types.hpp"
#include "strikezone/ball_tracker.hpp"

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace sz {

namespace {

// Epsilon for floating-point comparisons
constexpr double kEps = 1e-6;

// Simple 2D cross product of (b - a) x (c - a)
double cross(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c)
{
    const double abx = static_cast<double>(b.x - a.x);
    const double aby = static_cast<double>(b.y - a.y);
    const double acx = static_cast<double>(c.x - a.x);
    const double acy = static_cast<double>(c.y - a.y);
    return abx * acy - aby * acx;
}

// Point-in-polygon using ray casting.
// Returns true if point is inside or on boundary of polygon.
bool pointInPolygon(const cv::Point2f& p, const std::vector<cv::Point2f>& poly)
{
    const int n = static_cast<int>(poly.size());
    if (n < 3) {
        return false;
    }

    bool inside = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        const cv::Point2f& pi = poly[i];
        const cv::Point2f& pj = poly[j];

        // Check if edge (pj, pi) straddles the horizontal ray to +inf
        const bool intersect =
            ((pi.y > p.y) != (pj.y > p.y)) &&
            (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y + static_cast<float>(kEps)) + pi.x);

        if (intersect) {
            inside = !inside;
        }
    }

    return inside;
}

// Check if two line segments (p1, p2) and (q1, q2) intersect and, if so,
// compute the intersection point and the parameter t along segment (p1, p2)
// such that intersect = p1 + t*(p2 - p1), t in [0, 1].
bool segmentSegmentIntersection(const cv::Point2f& p1,
                                const cv::Point2f& p2,
                                const cv::Point2f& q1,
                                const cv::Point2f& q2,
                                double& outT,
                                cv::Point2f& outPoint)
{
    const double x1 = p1.x;
    const double y1 = p1.y;
    const double x2 = p2.x;
    const double y2 = p2.y;

    const double x3 = q1.x;
    const double y3 = q1.y;
    const double x4 = q2.x;
    const double y4 = q2.y;

    const double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(denom) < kEps) {
        // Parallel or nearly parallel
        return false;
    }

    const double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    const double u = ((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / denom;

    if (t < -kEps || t > 1.0 + kEps || u < -kEps || u > 1.0 + kEps) {
        return false;
    }

    // Clamp t to [0,1] for safety
    const double tClamped = std::min(1.0, std::max(0.0, t));
    outT = tClamped;

    const float ix = static_cast<float>(x1 + tClamped * (x2 - x1));
    const float iy = static_cast<float>(y1 + tClamped * (y2 - y1));
    outPoint = cv::Point2f(ix, iy);

    return true;
}

} // namespace

std::optional<PitchEvent> DecisionEngine::evaluatePitch(
    const std::vector<BallState>& trajectory,
    const StrikeZoneBounds& zone) const
{
    // Need at least two samples to form a trajectory segment
    if (trajectory.size() < 2 || zone.polygon.size() < 3) {
        return std::nullopt;
    }

    const auto& poly = zone.polygon;

    bool hasIntersection = false;
    double bestTGlobal   = std::numeric_limits<double>::max();
    cv::Point2f bestPoint;
    double bestTime      = 0.0;

    // Iterate over consecutive trajectory segments
    for (std::size_t i = 0; i + 1 < trajectory.size(); ++i) {
        const BallState& s0 = trajectory[i];
        const BallState& s1 = trajectory[i + 1];

        const cv::Point2f& p0 = s0.position;
        const cv::Point2f& p1 = s1.position;

        const bool inside0 = pointInPolygon(p0, poly);
        const bool inside1 = pointInPolygon(p1, poly);

        // If both inside, treat the entry as the earlier point
        if (inside0 && inside1) {
            const double t0 = s0.timestamp;
            // Choose the earliest such entry
            if (!hasIntersection || t0 < bestTime) {
                hasIntersection = true;
                bestTime = t0;
                bestPoint = p0;
                bestTGlobal = 0.0;
            }
            continue;
        }

        // If one is inside and the other outside, or segment crosses boundary,
        // check intersection with each polygon edge.
        double bestTForSegment = std::numeric_limits<double>::max();
        cv::Point2f bestPtForSegment;
        bool segmentHit = false;

        const int n = static_cast<int>(poly.size());
        for (int j = 0; j < n; ++j) {
            const cv::Point2f& q0 = poly[j];
            const cv::Point2f& q1 = poly[(j + 1) % n];

            double tSeg = 0.0;
            cv::Point2f ip;
            if (segmentSegmentIntersection(p0, p1, q0, q1, tSeg, ip)) {
                if (tSeg < bestTForSegment) {
                    bestTForSegment = tSeg;
                    bestPtForSegment = ip;
                    segmentHit = true;
                }
            }
        }

        if (segmentHit) {
            const double dt   = s1.timestamp - s0.timestamp;
            const double time = s0.timestamp + bestTForSegment * dt;

            if (!hasIntersection || time < bestTime) {
                hasIntersection = true;
                bestTime  = time;
                bestPoint = bestPtForSegment;
                bestTGlobal = bestTForSegment;
            }
        }
    }

    PitchEvent evt{};

    if (hasIntersection) {
        // The ball's path intersected the strike zone polygon → strike
        evt.isStrike      = true;
        evt.crossingTime  = bestTime;
        evt.crossingPoint = bestPoint;
        return evt;
    }

    // No geometric intersection with zone → ball.
    // We still emit a PitchEvent so the caller has timing info for logging.
    const BallState& last = trajectory.back();
    evt.isStrike      = false;
    evt.crossingTime  = last.timestamp;
    evt.crossingPoint = last.position;

    return evt;
}

} // namespace sz
