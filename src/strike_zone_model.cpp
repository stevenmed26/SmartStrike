#include "strikezone/strike_zone_model.hpp"
#include "strikezone/config.hpp"
#include "strikezone/types.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace sz {

StrikeZoneModel::StrikeZoneModel(const StrikeZoneConfig& cfg)
    : cfg_(cfg)
{
}

// Heuristic strike-zone model:
//
// Inputs:
//   - platePoly: polygon (in image coordinates) approximating home plate
//   - batterTopY: approximate y of batter's upper torso/head
//   - batterBottomY: approximate y of batter's feet / lower body
//
// We:
//
// 1) Use plate polygon to determine horizontal position & width of the zone
// 2) Use batter top/bottom and cfg_.topFactor / cfg_.bottomFactor
//    to determine vertical bounds of the zone
//
// All coordinates are in image space (pixels).
StrikeZoneBounds StrikeZoneModel::computeZone(const std::vector<cv::Point2f>& platePoly,
                                              double batterTopY,
                                              double batterBottomY) const
{
    StrikeZoneBounds zone{};

    if (platePoly.empty()) {
        return zone;
    }

    // --- 1. Derive horizontal span from plate polygon ---

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();

    for (const auto& p : platePoly) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
    }

    // If something went wrong, bail out with empty zone
    if (minX >= maxX) {
        return zone;
    }

    // Center and half-width from plate
    const float plateCenterX = 0.5f * (minX + maxX);
    const float plateHalfWidth = 0.5f * (maxX - minX);

    // Add a modest horizontal expansion factor so zone is slightly wider
    // than the visible plate footprint (catches edge pitches visually).
    constexpr float kHorizontalExpansionFactor = 1.15f;
    const float zoneHalfWidth = plateHalfWidth * kHorizontalExpansionFactor;

    const float zoneLeftX  = plateCenterX - zoneHalfWidth;
    const float zoneRightX = plateCenterX + zoneHalfWidth;

    // --- 2. Derive vertical span from batter height and configuration ---

    // Ensure proper ordering: top should have smaller y than bottom in image coordinates.
    double topY    = std::min(batterTopY, batterBottomY);
    double bottomY = std::max(batterTopY, batterBottomY);

    const double batterHeight = bottomY - topY;
    if (batterHeight <= 1e-3) {
        // Degenerate case, cannot compute a meaningful zone
        return zone;
    }

    // cfg_.topFactor and cfg_.bottomFactor are relative to batter height.
    //
    // Example:
    //   top_factor = 0.35   → zone_top = topY + 0.35 * batterHeight
    //   bottom_factor = 0.15 → zone_bottom = bottomY - 0.15 * batterHeight
    //
    const double zoneTopY =
        topY + cfg_.topFactor * batterHeight;

    const double zoneBottomY =
        bottomY - cfg_.bottomFactor * batterHeight;

    if (zoneBottomY <= zoneTopY) {
        // If configuration produced an invalid vertical range, bail out.
        return zone;
    }

    // --- 3. Construct a simple 4-vertex polygon for the strike zone ---

    zone.polygon.clear();
    zone.polygon.reserve(4);

    // In image coordinates: y increases downward.
    // We define polygon in clockwise order:
    //   top-left, top-right, bottom-right, bottom-left.
    zone.polygon.emplace_back(zoneLeftX,  static_cast<float>(zoneTopY));
    zone.polygon.emplace_back(zoneRightX, static_cast<float>(zoneTopY));
    zone.polygon.emplace_back(zoneRightX, static_cast<float>(zoneBottomY));
    zone.polygon.emplace_back(zoneLeftX,  static_cast<float>(zoneBottomY));

    return zone;
}

} // namespace sz

