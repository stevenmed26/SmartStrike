#pragma once

#include <string>

namespace sz {

struct CameraConfig {
    int deviceId;
    int width;
    int height;
    double fps;
};

struct DetectionConfig {
    double ballMinRadius;
    double ballMaxRadius;
    double minBallSpeed;
    int historyLength;
};

struct StrikeZoneConfig {
    double topFactor;     // relative to batter height
    double bottomFactor;  // relative to batter height
};

class Config {
public:
    // rootPath is treated as directory containing camera.yaml and model_params.yaml
    explicit Config(const std::string& rootPath);

    const CameraConfig& camera() const { return camera_; }
    const DetectionConfig& detection() const { return detection_; }
    const StrikeZoneConfig& zone() const { return zone_; }

private:
    CameraConfig camera_;
    DetectionConfig detection_;
    StrikeZoneConfig zone_;
};

} // namespace sz

