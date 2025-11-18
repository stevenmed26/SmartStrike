#include "strikezone/pipeline.hpp"
#include "strikezone/logger.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

namespace sz {

Pipeline::Pipeline(const Config& cfg)
    : cfg_(cfg)
    , camera_(cfg_.camera())
    , frameProcessor_()
    , plateDetector_()
    , batterDetector_()
    , zoneModel_(cfg_.zone())
    , ballDetector_()
    , ballTracker_()
    , decisionEngine_()
    , visualizer_()
    , currentZone_()
    , zoneInitialized_(false)
{
    // Nothing else to do here; components are default-constructed.
}

// -------------------- Public API --------------------

void Pipeline::runRealtime()
{
    if (!camera_.isOpen()) {
        throw std::runtime_error("Pipeline::runRealtime: camera is not open");
    }

    Logger::instance().info("Starting realtime Strike Zone Detector pipeline");

    cv::namedWindow("Strike Zone Detector", cv::WINDOW_NORMAL);

    const auto startTime = std::chrono::steady_clock::now();

    while (true) {
        Frame rawFrame;
        if (!camera_.read(rawFrame) || rawFrame.empty()) {
            Logger::instance().warn("Pipeline::runRealtime: failed to read frame from camera");
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        const double timestampSec =
            std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime).count();

        // Preprocess frame for downstream modules
        Frame processed = frameProcessor_.preprocess(rawFrame);

        // Process the frame (calibration, detection, decision) in-place
        processFrame(processed, timestampSec);

        // Display
        cv::imshow("Strike Zone Detector", processed);

        // Exit on ESC or 'q'
        const int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') {
            Logger::instance().info("Pipeline::runRealtime: exit requested by user");
            break;
        }
    }

    cv::destroyAllWindows();
}

void Pipeline::runOffline(const std::string& videoPath)
{
    Logger::instance().info("Starting offline Strike Zone Detector pipeline on: " + videoPath);

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        throw std::runtime_error("Pipeline::runOffline: failed to open video file: " + videoPath);
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 1e-3) {
        fps = 30.0; // fallback
        Logger::instance().warn("Pipeline::runOffline: FPS not available, defaulting to 30 FPS");
    }

    const int delayMs = static_cast<int>(1000.0 / fps);
    std::size_t frameIndex = 0;

    cv::namedWindow("Strike Zone Detector", cv::WINDOW_NORMAL);

    while (true) {
        Frame frame;
        if (!cap.read(frame) || frame.empty()) {
            Logger::instance().info("Pipeline::runOffline: end of video stream");
            break;
        }

        const double timestampSec = static_cast<double>(frameIndex) / fps;

        Frame processed = frameProcessor_.preprocess(frame);
        processFrame(processed, timestampSec);

        cv::imshow("Strike Zone Detector", processed);

        const int key = cv::waitKey(delayMs);
        if (key == 27 || key == 'q' || key == 'Q') {
            Logger::instance().info("Pipeline::runOffline: exit requested by user");
            break;
        }

        ++frameIndex;
    }

    cv::destroyAllWindows();
}

// -------------------- Internal helpers --------------------

void Pipeline::processFrame(Frame& frame, double timestamp)
{
    if (frame.empty()) {
        return;
    }

    // Work directly on this frame (draw overlays on it)
    Frame& drawable = frame;

    // 1. Initialize strike zone if not yet calibrated
    if (!zoneInitialized_) {
        const std::vector<cv::Point2f> platePoly = plateDetector_.detectPlate(drawable);

        if (!platePoly.empty()) {
            double batterTopY = 0.0;
            double batterBottomY = 0.0;
            const bool batterOk = batterDetector_.detectBatter(drawable, batterTopY, batterBottomY);

            if (batterOk) {
                currentZone_ = zoneModel_.computeZone(platePoly, batterTopY, batterBottomY);
                zoneInitialized_ = true;

                Logger::instance().info("Pipeline::processFrame: strike zone initialized");
            }

            // Draw plate even before full zone init for visualization
            visualizer_.drawPlate(drawable, platePoly);
        }

        // If zone is not initialized yet, we still show plate (if any) but skip pitch evaluation.
        if (!zoneInitialized_) {
            return;
        }
    }

    // 2. Draw strike zone
    visualizer_.drawStrikeZone(drawable, currentZone_);

    // 3. Detect ball in current frame
    const auto ballOpt = ballDetector_.detect(drawable);
    if (ballOpt) {
        // Track ball trajectory over time
        ballTracker_.addDetection(*ballOpt, timestamp);

        // Visualize detected ball
        visualizer_.drawBall(drawable, *ballOpt);
    }

    // 4. Evaluate pitch decision once we have a meaningful trajectory
    const auto trajOpt = ballTracker_.trajectory();
    if (trajOpt) {
        const auto eventOpt = decisionEngine_.evaluatePitch(*trajOpt, currentZone_);
        if (eventOpt) {
            const PitchEvent& evt = *eventOpt;

            // Visual overlay of decision
            visualizer_.annotateDecision(drawable, evt);

            // Log to console
            Logger::instance().info(
                std::string("Pitch decision: ") +
                (evt.isStrike ? "STRIKE" : "BALL") +
                " at t=" + std::to_string(evt.crossingTime) +
                "s, point=(" + std::to_string(evt.crossingPoint.x) +
                ", " + std::to_string(evt.crossingPoint.y) + ")"
            );

            // Note: We rely on BallTracker's internal logic (gaps/jumps) to
            // separate subsequent pitches. If you want a hard reset per pitch,
            // you can extend BallTracker with a reset() method and call it here.
        }
    }
}

} // namespace sz

