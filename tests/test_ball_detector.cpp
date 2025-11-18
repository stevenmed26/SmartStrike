#include <gtest/gtest.h>

#include "strikezone/ball_detector.hpp"
#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

using namespace sz;

namespace {

Frame makeBlankFrame(int width = 640, int height = 480)
{
    return Frame(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
}

} // namespace

TEST(BallDetectorTest, NoBallInBlankFrame)
{
    BallDetector detector;

    Frame frame = makeBlankFrame();
    auto result = detector.detect(frame);

    EXPECT_FALSE(result.has_value());
}

TEST(BallDetectorTest, DetectsSingleCenteredBall)
{
    BallDetector detector;

    const int width  = 640;
    const int height = 480;
    Frame frame = makeBlankFrame(width, height);

    // Draw a white ball in the center of the frame
    const cv::Point center(width / 2, height / 2);
    const int radius = 12; // within [4, 22] from our detector config

    cv::circle(frame, center, radius, cv::Scalar(255, 255, 255), cv::FILLED, cv::LINE_AA);

    auto result = detector.detect(frame);
    ASSERT_TRUE(result.has_value()) << "BallDetector should detect the synthetic ball";

    const BallDetection& det = *result;

    EXPECT_NEAR(det.center.x, static_cast<float>(center.x), 3.0f);
    EXPECT_NEAR(det.center.y, static_cast<float>(center.y), 3.0f);
    EXPECT_NEAR(det.radius, static_cast<float>(radius), 3.0f);
}

TEST(BallDetectorTest, IgnoresNonCircularObjects)
{
    BallDetector detector;

    Frame frame = makeBlankFrame(640, 480);

    // Draw a rectangle instead of a circle
    cv::rectangle(frame,
                  cv::Point(200, 200),
                  cv::Point(260, 260),
                  cv::Scalar(255, 255, 255),
                  cv::FILLED,
                  cv::LINE_AA);

    auto result = detector.detect(frame);

    // It might still occasionally detect a circle, but with our circularity check
    // we expect it to filter out such shapes most of the time.
    // This assertion assumes our circularity threshold is reasonably strict.
    EXPECT_FALSE(result.has_value());
}
