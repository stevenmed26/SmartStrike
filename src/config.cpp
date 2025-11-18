#include "strikezone/config.hpp"

#include <yaml-cpp/yaml.h>

#include <stdexcept>
#include <string>
#include <sstream>

namespace sz {

namespace {

// Helper to build a good error message
std::runtime_error makeConfigError(const std::string& msg, const std::string& path) {
    std::ostringstream oss;
    oss << "Config: " << msg << " (path: " << path << ")";
    return std::runtime_error(oss.str());
}

// Safe getter with default
template <typename T>
T getOrDefault(const YAML::Node& node, const std::string& key, const T& def) {
    if (!node || !node[key]) {
        return def;
    }
    return node[key].as<T>(def);
}

} // namespace

Config::Config(const std::string& rootPath)
{
    // In this implementation, `rootPath` is treated as a directory containing:
    //   rootPath + "/camera.yaml"
    //   rootPath + "/model_params.yaml"
    //
    // This matches the earlier example configuration layout:
    //   config/
    //     camera.yaml
    //     model_params.yaml
    //
    // If you want to pass a single file instead, you can adjust this logic.

    const std::string cameraPath      = rootPath + "/camera.yaml";
    const std::string modelParamsPath = rootPath + "/model_params.yaml";

    YAML::Node cameraNode;
    YAML::Node modelNode;

    try {
        cameraNode = YAML::LoadFile(cameraPath);
    } catch (const std::exception& e) {
        throw makeConfigError(std::string("failed to load camera config: ") + e.what(), cameraPath);
    }

    try {
        modelNode = YAML::LoadFile(modelParamsPath);
    } catch (const std::exception& e) {
        throw makeConfigError(std::string("failed to load model params config: ") + e.what(), modelParamsPath);
    }

    // ---- CameraConfig ----
    // Expected structure (camera.yaml):
    //
    // device_id: 0
    // width: 1920
    // height: 1080
    // fps: 60
    //
    CameraConfig cam{};
    cam.deviceId = getOrDefault<int>(cameraNode, "device_id", 0);
    cam.width    = getOrDefault<int>(cameraNode, "width", 1280);
    cam.height   = getOrDefault<int>(cameraNode, "height", 720);
    cam.fps      = getOrDefault<double>(cameraNode, "fps", 60.0);
    camera_ = cam;

    // ---- DetectionConfig ----
    // Expected structure (model_params.yaml):
    //
    // ball_detection:
    //   min_radius: 4
    //   max_radius: 22
    //   min_motion_speed: 1.2
    //
    // tracking:
    //   max_history: 80
    //
    DetectionConfig det{};
    const YAML::Node ballDet = modelNode["ball_detection"];
    const YAML::Node tracking = modelNode["tracking"];

    if (!ballDet || !ballDet.IsMap()) {
        throw makeConfigError("missing or invalid 'ball_detection' section", modelParamsPath);
    }

    det.ballMinRadius = getOrDefault<double>(ballDet, "min_radius", 4.0);
    det.ballMaxRadius = getOrDefault<double>(ballDet, "max_radius", 22.0);
    det.minBallSpeed  = getOrDefault<double>(ballDet, "min_motion_speed", 1.0);

    det.historyLength = getOrDefault<int>(tracking, "max_history", 80);
    detection_ = det;

    // ---- StrikeZoneConfig ----
    // Expected structure (model_params.yaml):
    //
    // strike_zone:
    //   top_factor: 0.35
    //   bottom_factor: 0.15
    //
    StrikeZoneConfig zone{};
    const YAML::Node zoneNode = modelNode["strike_zone"];

    if (!zoneNode || !zoneNode.IsMap()) {
        throw makeConfigError("missing or invalid 'strike_zone' section", modelParamsPath);
    }

    zone.topFactor    = getOrDefault<double>(zoneNode, "top_factor", 0.35);
    zone.bottomFactor = getOrDefault<double>(zoneNode, "bottom_factor", 0.15);
    zone_ = zone;
}

} // namespace sz
