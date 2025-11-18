#include "strikezone/visualizer.hpp"
#include "strikezone/types.hpp"
#include "strikezone/ball_detector.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace sz {

namespace {

// Drawing constants. You can later wire these to model_params.yaml if desired.
const cv::Scalar kPlateColor   = cv::Scalar(255, 255, 255); // white
const cv::Scalar kZoneOutline  = cv::Scalar(255, 128, 0);   // blue-ish/orange-ish
const cv::Scalar kZoneFill     = cv::Scalar(255, 128, 0);   // same hue for fill
const cv::Scalar kBallColor    = cv::Scalar(0, 255, 255);   // yellow
const cv::Scalar kStrikeColor  = cv::Scalar(0, 255, 0);     // green
const cv::Scalar kBallTextColor= cv::Scalar(0, 0, 255);     // red
const cv::Scalar kShadowColor  = cv::Scalar(0, 0, 0);       // black for text shadow

constexpr int   kPlateThickness      = 2;
constexpr int   kZoneThickness       = 2;
constexpr int   kTrajectoryThickness = 2;
constexpr int   kBallThickness       = 2;
constexpr int   kCrossMarkerSize     = 6;

constexpr double kZoneFillAlpha      = 0.25; // transparency for zone fill [0..1]
constexpr int    kFontFace           = cv::FONT_HERSHEY_SIMPLEX;
constexpr double kFontScale          = 0.7;
constexpr int    kFontThickness      = 2;

// Draw text with a slight shadow so it stands out against the background.
void putTextWithShadow(cv::Mat& img,
                       const std::string& text,
                       const cv::Point& org,
                       const cv::Scalar& color,
                       int fontFace,
                       double fontScale,
                       int thickness)
{
    const cv::Point shadowOrg(org.x + 1, org.y + 1);

    // Shadow
    cv::putText(img, text, shadowOrg, fontFace, fontScale, kShadowColor, thickness + 1, cv::LINE_AA);

    // Foreground
    cv::putText(img, text, org, fontFace, fontScale, color, thickness, cv::LINE_AA);
}

// Draw a filled polygon overlay with alpha blending.
void drawFilledPolygonOverlay(cv::Mat& img,
                              const std::vector<cv::Point>& poly,
                              const cv::Scalar& color,
                              double alpha)
{
    if (poly.empty() || alpha <= 0.0) {
        return;
    }

    cv::Mat overlay = img.clone();
    std::vector<std::vector<cv::Point>> polys;
    polys.push_back(poly);

    cv::fillPoly(overlay, polys, color);

    // Blend overlay into original image
    cv::addWeighted(overlay, alpha, img, 1.0 - alpha, 0.0, img);
}

} // namespace

void Visualizer::drawPlate(Frame& frame, const std::vector<cv::Point2f>& platePoly) const
{
    if (frame.empty() || platePoly.size() < 2) {
        return;
    }

    std::vector<cv::Point> pts;
    pts.reserve(platePoly.size());
    for (const auto& p : platePoly) {
        pts.emplace_back(cv::Point(static_cast<int>(std::round(p.x)),
                                   static_cast<int>(std::round(p.y))));
    }

    const std::vector<std::vector<cv::Point>> contours{pts};

    cv::polylines(frame,
                  contours,
                  true,               // closed
                  kPlateColor,
                  kPlateThickness,
                  cv::LINE_AA);
}

void Visualizer::drawStrikeZone(Frame& frame, const StrikeZoneBounds& zone) const
{
    if (frame.empty() || zone.polygon.size() < 3) {
        return;
    }

    std::vector<cv::Point> pts;
    pts.reserve(zone.polygon.size());
    for (const auto& p : zone.polygon) {
        pts.emplace_back(cv::Point(static_cast<int>(std::round(p.x)),
                                   static_cast<int>(std::round(p.y))));
    }

    // Filled, semi-transparent zone
    drawFilledPolygonOverlay(frame, pts, kZoneFill, kZoneFillAlpha);

    const std::vector<std::vector<cv::Point>> contours{pts};

    // Outline
    cv::polylines(frame,
                  contours,
                  true,
                  kZoneOutline,
                  kZoneThickness,
                  cv::LINE_AA);
}

void Visualizer::drawBall(Frame& frame, const BallDetection& ball) const
{
    if (frame.empty()) {
        return;
    }

    const cv::Point center(static_cast<int>(std::round(ball.center.x)),
                           static_cast<int>(std::round(ball.center.y)));
    const int radius = std::max(1, static_cast<int>(std::round(ball.radius)));

    cv::circle(frame,
               center,
               radius,
               kBallColor,
               kBallThickness,
               cv::LINE_AA);

    // Small center marker
    cv::circle(frame,
               center,
               2,
               kBallColor,
               cv::FILLED,
               cv::LINE_AA);
}

void Visualizer::annotateDecision(Frame& frame, const PitchEvent& event) const
{
    if (frame.empty()) {
        return;
    }

    const std::string label = event.isStrike ? "STRIKE" : "BALL";
    const cv::Scalar color  = event.isStrike ? kStrikeColor : kBallTextColor;

    // Draw decision text near the top-left corner of the frame
    const int margin = 16;
    cv::Point textOrg(margin, margin + 24);

    putTextWithShadow(frame,
                      label,
                      textOrg,
                      color,
                      kFontFace,
                      kFontScale,
                      kFontThickness);

    // Optionally add timing info as smaller text below
    {
        std::string timeStr = "t = " + std::to_string(event.crossingTime) + " s";
        cv::Point timeOrg(margin, margin + 24 + 24);

        putTextWithShadow(frame,
                          timeStr,
                          timeOrg,
                          color,
                          kFontFace,
                          0.5,
                          1);
    }

    // Mark the crossing point if it is inside the frame bounds
    const int width  = frame.cols;
    const int height = frame.rows;

    const int x = static_cast<int>(std::round(event.crossingPoint.x));
    const int y = static_cast<int>(std::round(event.crossingPoint.y));

    if (x >= 0 && x < width && y >= 0 && y < height) {
        cv::Point center(x, y);

        // Draw a small crosshair at the crossing point
        cv::line(frame,
                 cv::Point(center.x - kCrossMarkerSize, center.y),
                 cv::Point(center.x + kCrossMarkerSize, center.y),
                 color,
                 2,
                 cv::LINE_AA);

        cv::line(frame,
                 cv::Point(center.x, center.y - kCrossMarkerSize),
                 cv::Point(center.x, center.y + kCrossMarkerSize),
                 color,
                 2,
                 cv::LINE_AA);
    }
}

} // namespace sz
