#include "strikezone/batter_detector.hpp"
#include "strikezone/types.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace sz {

namespace {

// Heuristics for detecting the batter's silhouette using contours.
// These can be wired to config later if needed.

constexpr int   kBlurKernelSize        = 5;
constexpr double kCannyThreshold1      = 40.0;
constexpr double kCannyThreshold2      = 120.0;

constexpr double kMinBatterAreaFrac    = 0.02;  // min contour area as fraction of image area
constexpr double kMaxBatterAreaFrac    = 0.60;  // max contour area as fraction of image area

constexpr double kMinAspectRatio       = 1.2;   // height / width
constexpr double kMaxAspectRatio       = 6.0;

constexpr double kCenterRegionMinFrac  = 0.15;  // how close to horizontal center (as fraction)
constexpr double kCenterRegionMaxFrac  = 0.85;

constexpr int   kMorphKernelSize       = 5;
constexpr int   kMorphIterations       = 1;

// Compute a "batter score" for a bounding box: prioritize tall, central contours.
double scoreBoundingBox(const cv::Rect& bbox, int imgWidth, int imgHeight)
{
    const double height = static_cast<double>(bbox.height);
    const double width  = static_cast<double>(bbox.width);

    if (width <= 0.0 || height <= 0.0) {
        return 0.0;
    }

    const double aspect = height / width;
    if (aspect < kMinAspectRatio || aspect > kMaxAspectRatio) {
        return 0.0;
    }

    const double area = width * height;
    const double imgArea = static_cast<double>(imgWidth * imgHeight);
    const double areaFrac = area / imgArea;

    if (areaFrac < kMinBatterAreaFrac || areaFrac > kMaxBatterAreaFrac) {
        return 0.0;
    }

    // Horizontal centrality: closer to center is better.
    const double centerX = static_cast<double>(bbox.x + bbox.width / 2);
    const double normCenterX = centerX / static_cast<double>(imgWidth); // 0..1

    if (normCenterX < kCenterRegionMinFrac || normCenterX > kCenterRegionMaxFrac) {
        return 0.0;
    }

    // Score: weighted sum of normalized height and area.
    const double normHeight = height / static_cast<double>(imgHeight);
    const double score = 0.7 * normHeight + 0.3 * areaFrac;

    return score;
}

} // namespace

bool BatterDetector::detectBatter(const Frame& frame, double& topY, double& bottomY) const
{
    if (frame.empty()) {
        return false;
    }

    const int imgWidth  = frame.cols;
    const int imgHeight = frame.rows;
    if (imgWidth <= 0 || imgHeight <= 0) {
        return false;
    }

    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = frame.clone();
    }

    // Preprocessing: blur and equalize to stabilize edges.
    cv::GaussianBlur(gray, gray, cv::Size(kBlurKernelSize, kBlurKernelSize), 1.5);
    cv::equalizeHist(gray, gray);

    // Binary/edge representation
    cv::Mat edges;
    cv::Canny(gray, edges, kCannyThreshold1, kCannyThreshold2);

    // Slight morphological closing to fill gaps in the silhouette.
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
                                               cv::Size(kMorphKernelSize, kMorphKernelSize));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), kMorphIterations);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return false;
    }

    double bestScore = 0.0;
    cv::Rect bestBBox;
    bool found = false;

    for (const auto& contour : contours) {
        if (contour.size() < 8) {
            continue;
        }

        cv::Rect bbox = cv::boundingRect(contour);
        if (bbox.width <= 0 || bbox.height <= 0) {
            continue;
        }

        const double s = scoreBoundingBox(bbox, imgWidth, imgHeight);
        if (s > bestScore) {
            bestScore = s;
            bestBBox = bbox;
            found = true;
        }
    }

    if (!found || bestScore <= 0.0) {
        return false;
    }

    // Map bounding box to approximate torso region.
    // We assume the torso occupies roughly middle 60% of the bounding box vertically.
    const double bboxTop    = static_cast<double>(bestBBox.y);
    const double bboxBottom = static_cast<double>(bestBBox.y + bestBBox.height);

    // You could refine further with a torso-specific fraction; for now we use the box.
    topY    = bboxTop;
    bottomY = bboxBottom;

    return true;
}

} // namespace sz
