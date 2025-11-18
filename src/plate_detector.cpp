#include "strikezone/plate_detector.hpp"
#include "strikezone/types.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace sz {

namespace
{
// These parameters roughly correspond to the calibration example we discussed
// (calibration.yaml → plate_detection section). You can later wire them up to
// actual YAML config instead of hardcoding here.

constexpr int   kBlurKernelSize        = 5;
constexpr double kCannyThreshold1      = 40.0;
constexpr double kCannyThreshold2      = 110.0;

constexpr double kMinPlateAreaFrac     = 0.008;  // fraction of image area
constexpr double kMaxPlateAreaFrac     = 0.08;   // fraction of image area

constexpr double kPolygonApproxEpsilonFrac = 0.02; // as fraction of perimeter

// Heuristic: plate should be near bottom-center of the frame
constexpr double kMinCenterYFrac       = 0.45;
constexpr double kMaxCenterYFrac       = 0.98;
constexpr double kMinCenterXFrac       = 0.20;
constexpr double kMaxCenterXFrac       = 0.80;

// Minimum and maximum acceptable number of vertices after polygon approximation
constexpr int kMinVertices             = 4;
constexpr int kMaxVertices             = 7;

// Compute a simple score for how "plate-like" a contour's bounding box is.
double scorePlateCandidate(const cv::Rect& bbox,
                           int imgWidth,
                           int imgHeight,
                           int approxVertexCount)
{
    const double w = static_cast<double>(bbox.width);
    const double h = static_cast<double>(bbox.height);

    if (w <= 0.0 || h <= 0.0) {
        return 0.0;
    }

    const double area = w * h;
    const double imgArea = static_cast<double>(imgWidth * imgHeight);
    const double areaFrac = area / imgArea;

    if (areaFrac < kMinPlateAreaFrac || areaFrac > kMaxPlateAreaFrac) {
        return 0.0;
    }

    // Plate is wider than it is tall (from typical broadcast angle).
    const double aspect = w / h;
    if (aspect < 0.7 || aspect > 3.5) {
        return 0.0;
    }

    // Center position
    const double centerX = bbox.x + w / 2.0;
    const double centerY = bbox.y + h / 2.0;

    const double normX = centerX / static_cast<double>(imgWidth);
    const double normY = centerY / static_cast<double>(imgHeight);

    if (normY < kMinCenterYFrac || normY > kMaxCenterYFrac) {
        return 0.0;
    }

    if (normX < kMinCenterXFrac || normX > kMaxCenterXFrac) {
        return 0.0;
    }

    // Bonus if vertex count is close to 5 (home plate is a pentagon)
    double vertexScore = 1.0;
    const int targetVertices = 5;
    const int diff = std::abs(approxVertexCount - targetVertices);
    if (diff == 0) {
        vertexScore = 1.2;
    } else if (diff == 1) {
        vertexScore = 1.0;
    } else {
        vertexScore = 0.8;
    }

    // Final score: area fraction + centrality bonus + vertex bonus.
    // Higher area and more central → higher score.
    const double centralityScore = 1.0 - std::abs(normY - 0.8); // prefer near bottom (0.8)
    const double score = (0.6 * areaFrac + 0.4 * centralityScore) * vertexScore;

    return score;
}

// Order polygon vertices in clockwise order around their centroid.
// This is helpful for consistent downstream usage/visualization.
std::vector<cv::Point2f> orderClockwise(const std::vector<cv::Point2f>& pts)
{
    if (pts.size() <= 2) {
        return pts;
    }

    cv::Point2f center(0.f, 0.f);
    for (const auto& p : pts) {
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= static_cast<float>(pts.size());
    center.y /= static_cast<float>(pts.size());

    std::vector<std::pair<double, cv::Point2f>> anglePoints;
    anglePoints.reserve(pts.size());

    for (const auto& p : pts) {
        const double angle = std::atan2(static_cast<double>(p.y - center.y),
                                        static_cast<double>(p.x - center.x));
        anglePoints.emplace_back(angle, p);
    }

    std::sort(anglePoints.begin(), anglePoints.end(),
              [](const auto& a, const auto& b) {
                  return a.first < b.first;
              });

    std::vector<cv::Point2f> ordered;
    ordered.reserve(pts.size());
    for (const auto& ap : anglePoints) {
        ordered.push_back(ap.second);
    }
    return ordered;
}

} // namespace

std::vector<cv::Point2f> PlateDetector::detectPlate(const Frame& frame) const
{
    std::vector<cv::Point2f> emptyResult;
    if (frame.empty()) {
        return emptyResult;
    }

    const int imgWidth  = frame.cols;
    const int imgHeight = frame.rows;
    if (imgWidth <= 0 || imgHeight <= 0) {
        return emptyResult;
    }

    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = frame.clone();
    }

    // Preprocessing: blur to reduce noise, then edge detection.
    cv::GaussianBlur(gray, gray, cv::Size(kBlurKernelSize, kBlurKernelSize), 1.5);
    cv::Mat edges;
    cv::Canny(gray, edges, kCannyThreshold1, kCannyThreshold2);

    // Optional: morphological close to connect fragmented edges.
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
                                               cv::Size(3, 3));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return emptyResult;
    }

    double bestScore = 0.0;
    std::vector<cv::Point2f> bestPoly;

    const double imgArea = static_cast<double>(imgWidth * imgHeight);

    for (const auto& contour : contours) {
        if (contour.size() < 6) {
            continue;
        }

        const double area = cv::contourArea(contour);
        if (area < imgArea * kMinPlateAreaFrac || area > imgArea * kMaxPlateAreaFrac) {
            continue;
        }

        const cv::Rect bbox = cv::boundingRect(contour);

        // Approximate polygon
        std::vector<cv::Point> approx;
        const double epsilon = kPolygonApproxEpsilonFrac * cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, epsilon, true);

        if (approx.size() < static_cast<std::size_t>(kMinVertices) ||
            approx.size() > static_cast<std::size_t>(kMaxVertices)) {
            // If polygon is too simple or too complex, skip
            continue;
        }

        // Score candidate
        const double s = scorePlateCandidate(bbox, imgWidth, imgHeight, static_cast<int>(approx.size()));
        if (s > bestScore) {
            bestScore = s;

            bestPoly.clear();
            bestPoly.reserve(approx.size());
            for (const auto& p : approx) {
                bestPoly.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y));
            }
        }
    }

    if (bestPoly.empty()) {
        return emptyResult;
    }

    // Order points for consistency
    return orderClockwise(bestPoly);
}

} // namespace sz
