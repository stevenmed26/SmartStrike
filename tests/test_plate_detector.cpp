#include <gtest/gtest.h>

#include "strikezone/plate_detector.hpp"
#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace sz;

namespace {

Frame makeBlankFrame(int width = 1280, int height = 720)
{
    return Frame(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
}

// Draw a synthetic "home plate" near the bottom-center of the frame.
// We use a simple 5-point polygon that roughly resembles a pentagon.
void drawSyntheticPlate(Frame& frame)
{
    const int width  = frame.cols;
    const int height = frame.rows;

    const int plateWidth  = 160;
    const int plateHeight = 120;

    const int centerX = width / 2;
    const int baseY   = static_cast<int>(height * 0.85);

    // Approximate home-plate coordinates
    std::vector<cv::Point> platePts;
    platePts.emplace_back(centerX - plateWidth / 2, baseY);                     // bottom-left
    platePts.emplace_back(centerX + plateWidth / 2, baseY);                     // bottom-right
    platePts.emplace_back(centerX + plateWidth / 2, baseY - plateHeight / 2);   // mid-right
    platePts.emplace_back(centerX,                 baseY - plateHeight);        // top
    platePts.emplace_back(centerX - plateWidth / 2, baseY - plateHeight / 2);   // mid-left

    const std::vector<std::vector<cv::Point>> polys{platePts};

    cv::fillPoly(frame, polys, cv::Scalar(255, 255, 255)); // white plate
}

} // namespace

TEST(PlateDetectorTest, NoPlateInBlankFrame)
{
    PlateDetector detector;
    Frame frame = makeBlankFrame();

    auto poly = detector.detectPlate(frame);
    EXPECT_TRUE(poly.empty());
}

TEST(PlateDetectorTest, DetectsSyntheticPlate)
{
    PlateDetector detector;

    Frame frame = makeBlankFrame();
    drawSyntheticPlate(frame);

    auto poly = detector.detectPlate(frame);

    // We don't enforce exact vertex count since approxPolyDP can vary,
    // but we expect a non-empty polygon with at least 4 vertices.
    ASSERT_FALSE(poly.empty()) << "PlateDetector should detect the synthetic plate";
    EXPECT_GE(poly.size(), 4u);
    EXPECT_LE(poly.size(), 7u);

    // The centroid of the detected plate should be near the bottom-center of the frame.
    float cx = 0.0f;
    float cy = 0.0f;
    for (const auto& p : poly) {
        cx += p.x;
        cy += p.y;
    }
    cx /= static_cast<float>(poly.size());
    cy /= static_cast<float>(poly.size());

    const float expectedX = frame.cols / 2.0f;
    const float expectedY = frame.rows * 0.85f;

    EXPECT_NEAR(cx, expectedX, 100.0f); // allow generous tolerance
    EXPECT_NEAR(cy, expectedY, 100.0f);
}
