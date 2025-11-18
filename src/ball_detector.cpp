#include "strikezone/ball_detector.hpp"
#include "strikezone/types.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace sz {

namespace {

// These constants should conceptually match values in model_params.yaml.
// You can expose them via configuration if you want runtime tunability.
constexpr float kMinRadiusPx       = 4.0f;
constexpr float kMaxRadiusPx       = 22.0f;
constexpr float kMinCircularity    = 0.65f;

// HoughCircles parameters
constexpr double kHoughDp          = 1.2;   // Inverse ratio of accumulator resolution
constexpr double kHoughMinDistPx   = 25.0;  // Minimum distance between detected centers
constexpr double kHoughParam1      = 120.0; // Upper threshold for internal Canny
constexpr double kHoughParam2      = 18.0;  // Accumulator threshold for circle centers

// Gaussian blur kernel size for pre-processing
constexpr int kBlurKernelSize      = 5;

// Margin around the circle when extracting ROI for circularity check
constexpr int kRoiMarginPx         = 3;

// Compute circularity of a candidate ball using contours in a local ROI:
// circularity = 4πA / P²   (1.0 is a perfect circle, lower is more elongated)
//
// gray    : preprocessed grayscale frame
// center  : candidate circle center in full-image coordinates
// radius  : candidate radius in pixels
//
// Returns a value in [0, 1], where higher means more circular.
float computeCircularity(const cv::Mat& gray,
                         const cv::Point2f& center,
                         float radius)
{
    if (gray.empty() || radius <= 0.0f) {
        return 0.0f;
    }

    const int imgWidth  = gray.cols;
    const int imgHeight = gray.rows;

    // Compute ROI bounds in image coordinates
    const int roiRadius = static_cast<int>(std::ceil(radius)) + kRoiMarginPx;
    int x0 = static_cast<int>(std::round(center.x)) - roiRadius;
    int y0 = static_cast<int>(std::round(center.y)) - roiRadius;
    int x1 = static_cast<int>(std::round(center.x)) + roiRadius;
    int y1 = static_cast<int>(std::round(center.y)) + roiRadius;

    // Clamp to image bounds
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, imgWidth - 1);
    y1 = std::min(y1, imgHeight - 1);

    if (x1 <= x0 || y1 <= y0) {
        return 0.0f;
    }

    cv::Rect roi(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
    cv::Mat patch = gray(roi).clone();
    if (patch.empty()) {
        return 0.0f;
    }

    // Edge detection to isolate ball contour
    cv::Mat edges;
    cv::Canny(patch, edges, 50, 150);

    // Find contours in the local patch
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return 0.0f;
    }

    // Choose the contour whose centroid is closest to the circle center
    double bestDistSq = std::numeric_limits<double>::max();
    int bestIdx       = -1;

    const cv::Point2f localCenter(center.x - static_cast<float>(roi.x),
                                  center.y - static_cast<float>(roi.y));

    for (int i = 0; i < static_cast<int>(contours.size()); ++i) {
        const auto& contour = contours[i];
        if (contour.size() < 5) {
            continue;
        }

        cv::Moments m = cv::moments(contour);
        if (std::abs(m.m00) < 1e-3) {
            continue;
        }

        cv::Point2f c(static_cast<float>(m.m10 / m.m00),
                      static_cast<float>(m.m01 / m.m00));

        const double dx = static_cast<double>(c.x - localCenter.x);
        const double dy = static_cast<double>(c.y - localCenter.y);
        const double distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) {
        return 0.0f;
    }

    const auto& bestContour = contours[bestIdx];
    const double area = cv::contourArea(bestContour);
    const double perimeter = cv::arcLength(bestContour, true);

    if (perimeter <= 1e-3 || area <= 1e-3) {
        return 0.0f;
    }

    const double circularity = 4.0 * CV_PI * area / (perimeter * perimeter);
    // Clamp to [0, 1] for robustness
    return static_cast<float>(std::max(0.0, std::min(1.0, circularity)));
}

} // namespace

std::optional<BallDetection> BallDetector::detect(const Frame& frame) const
{
    if (frame.empty()) {
        return std::nullopt;
    }

    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else if (frame.channels() == 4) {
        cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = frame.clone();
    }

    // Basic preprocessing: blur + histogram equalization
    cv::GaussianBlur(gray, gray, cv::Size(kBlurKernelSize, kBlurKernelSize), 1.5);
    cv::equalizeHist(gray, gray);

    // Hough circle detection
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(
        gray,
        circles,
        cv::HOUGH_GRADIENT,
        kHoughDp,
        kHoughMinDistPx,
        kHoughParam1,
        kHoughParam2,
        static_cast<int>(std::round(kMinRadiusPx)),
        static_cast<int>(std::round(kMaxRadiusPx))
    );

    if (circles.empty()) {
        return std::nullopt;
    }

    // Evaluate candidate circles by circularity and radius
    bool foundAny = false;
    float bestScore = 0.0f;
    BallDetection bestDetection{};

    for (const auto& c : circles) {
        const cv::Point2f center(c[0], c[1]);
        const float radius = c[2];

        // Sanity checks on radius
        if (radius < kMinRadiusPx || radius > kMaxRadiusPx) {
            continue;
        }

        // Compute circularity in the local region around this candidate
        const float circularity = computeCircularity(gray, center, radius);
        if (circularity < kMinCircularity) {
            continue;
        }

        // Score can combine circularity and radius (larger, rounder circles preferred)
        const float score = circularity * radius;

        if (!foundAny || score > bestScore) {
            foundAny = true;
            bestScore = score;
            bestDetection.center = center;
            bestDetection.radius = radius;
        }
    }

    if (!foundAny) {
        return std::nullopt;
    }

    return bestDetection;
}

} // namespace sz
