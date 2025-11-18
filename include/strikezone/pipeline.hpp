#pragma once

#include "strikezone/types.hpp"
#include "strikezone/config.hpp"
#include "strikezone/camera_capture.hpp"
#include "strikezone/frame_processor.hpp"
#include "strikezone/plate_detector.hpp"
#include "strikezone/batter_detector.hpp"
#include "strikezone/strike_zone_model.hpp"
#include "strikezone/ball_detector.hpp"
#include "strikezone/ball_tracker.hpp"
#include "strikezone/decision_engine.hpp"
#include "strikezone/visualizer.hpp"

#include <string>

namespace sz {

class Pipeline {
public:
    explicit Pipeline(const Config& cfg);

    // Main realtime loop using camera input
    void runRealtime();

    // Process a video file offline
    void runOffline(const std::string& videoPath);

private:
    Config cfg_;
    CameraCapture camera_;
    FrameProcessor frameProcessor_;
    PlateDetector plateDetector_;
    BatterDetector batterDetector_;
    StrikeZoneModel zoneModel_;
    BallDetector ballDetector_;
    BallTracker ballTracker_;
    DecisionEngine decisionEngine_;
    Visualizer visualizer_;

    StrikeZoneBounds currentZone_;
    bool zoneInitialized_;

    // Process a single frame in-place (draw overlays directly on this frame)
    void processFrame(Frame& frame, double timestamp);
};

} // namespace sz
