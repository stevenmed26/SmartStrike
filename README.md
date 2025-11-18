# SmartStrike


Strike Zone Detector

A high-performance C++17 + OpenCV computer-vision system that detects and classifies baseball pitches as strikes or balls in real time. The system uses geometric calibration, batter-specific strike-zone modeling, ball detection, trajectory estimation, and a modular pipeline architecture suitable for both realtime and offline analysis.

C++17 | OpenCV 4.x | CMake 3.15+ | Real-Time Enabled | MIT License

Overview

The Strike Zone Detector is a modular, testable computer-vision pipeline built in modern C++.

It can:

Capture frames from a camera or video file

Detect the home plate for calibration

Detect batter posture to personalize the strike zone

Detect and track baseballs in flight

Reconstruct trajectory across frames

Determine if a pitch crosses the strike zone

Annotate results visually with overlays and decision text

This project follows a clean, production-grade architecture with strong separation between detection, tracking, decision logic, and visualization.

Project Structure

strike-zone-detector/
├── CMakeLists.txt
├── README.md
├── config/
│ ├── camera.yaml
│ ├── calibration.yaml
│ └── model_params.yaml
├── data/
│ └── sample_video.mp4
├── include/
│ └── strikezone/
│ ├── types.hpp
│ ├── config.hpp
│ ├── logger.hpp
│ ├── camera_capture.hpp
│ ├── frame_processor.hpp
│ ├── plate_detector.hpp
│ ├── batter_detector.hpp
│ ├── strike_zone_model.hpp
│ ├── ball_detector.hpp
│ ├── ball_tracker.hpp
│ ├── decision_engine.hpp
│ ├── visualizer.hpp
│ └── pipeline.hpp
├── src/
│ ├── config.cpp
│ ├── logger.cpp
│ ├── camera_capture.cpp
│ ├── frame_processor.cpp
│ ├── plate_detector.cpp
│ ├── batter_detector.cpp
│ ├── strike_zone_model.cpp
│ ├── ball_detector.cpp
│ ├── ball_tracker.cpp
│ ├── decision_engine.cpp
│ ├── visualizer.cpp
│ └── pipeline.cpp
├── app/
│ ├── main_realtime.cpp
│ └── main_offline.cpp
└── tests/
├── test_ball_detector.cpp
├── test_plate_detector.cpp
└── test_decision_engine.cpp

System Architecture

Camera Capture
Captures frames from webcam or video file. Abstract input layer so sources are easily interchangeable.

Preprocessing
Performs grayscale conversion, histogram equalization, Gaussian blur, and noise filtering.

Plate Detection
Detects the 5-sided home plate polygon used as the reference geometry for the strike zone.

Batter Detection
Estimates batter upper and lower torso bounds to personalize the zone height.

Strike Zone Modeling
Computes a projected 2D polygon representing the strike zone using MLB height rules and plate geometry.

Ball Detection
Identifies candidate baseballs using contour shape, circularity, radius constraints, and motion.

Ball Tracking
Maintains a time-stamped trajectory of ball positions to detect where the ball crosses the strike zone.

Decision Engine
Computes trajectory/zone intersection and outputs a PitchEvent containing:

isStrike

crossingTime

crossingPoint

Visualization
Draws overlays for home plate, strike zone, ball detections, and strike/ball decisions.

Pipeline
Orchestrates all components in realtime or offline modes, handling initialization, timing, and sequencing.

Installation

Prerequisites:

C++17

CMake 3.15+

OpenCV 4.x

Build Instructions:

git clone https://github.com/your/repo.git

cd strike-zone-detector
mkdir build && cd build
cmake ..
make -j8

Running

Realtime Mode (webcam):

./strikezone_realtime

Offline Video Mode:

./strikezone_offline ../data/sample_video.mp4

Configuration Files

camera.yaml:
device_id: 0
width: 1280
height: 720
fps: 60

model_params.yaml:
ball_min_radius: 4
ball_max_radius: 25
min_ball_speed: 1.0
history_length: 60
strike_zone:
top_factor: 0.35
bottom_factor: 0.15

calibration.yaml:
Contains camera calibration and plate tuning parameters depending on your calibration method.

Testing

Unit tests (using GoogleTest):

cd build
ctest --verbose

Tests include:

Ball detection

Plate detection

Decision engine correctness

Example Output

You will see:

White outline = home plate

Blue polygon = strike zone

Yellow circle = baseball detection

Green text = STRIKE

Red text = BALL

Extending the System

The architecture is modular, allowing easy upgrades:

Ball Detection → YOLOv8, TensorRT

Batter Detection → MediaPipe Pose, OpenPose

Strike Zone Model → Full 3D geometric calibration

Tracking → Kalman Filter, Extended Kalman

Visualization → ImGUI, WebSocket streaming to browser UI

License

MIT License

Author

Steven Mediterraneo
Computer Vision • C++ • Backend Engineering