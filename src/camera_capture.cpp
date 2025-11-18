#include "strikezone/camera_capture.hpp"
#include "strikezone/config.hpp"
#include "strikezone/types.hpp"

#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>

#include <stdexcept>
#include <string>
#include <iostream>

namespace sz {

CameraCapture::CameraCapture(const CameraConfig& cfg)
    : cap_()
{
    // Open the camera device by ID
    if (!cap_.open(cfg.deviceId, cv::CAP_ANY)) {
        throw std::runtime_error(
            "CameraCapture: failed to open camera device with ID " +
            std::to_string(cfg.deviceId)
        );
    }

    // Configure basic properties: resolution and FPS (if provided)
    if (cfg.width > 0) {
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(cfg.width));
    }
    if (cfg.height > 0) {
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(cfg.height));
    }
    if (cfg.fps > 0.0) {
        cap_.set(cv::CAP_PROP_FPS, cfg.fps);
    }

    // Try to keep buffer small to reduce latency (ignored silently if unsupported)
#ifdef CV_CAP_PROP_BUFFERSIZE
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 2);
#endif

    // Verify camera actually delivers frames
    Frame testFrame;
    if (!cap_.read(testFrame) || testFrame.empty()) {
        cap_.release();
        throw std::runtime_error(
            "CameraCapture: camera opened but failed to read first frame"
        );
    }

    // Rewind by reopening to avoid consuming an initial frame
    cap_.release();
    if (!cap_.open(cfg.deviceId, cv::CAP_ANY)) {
        throw std::runtime_error(
            "CameraCapture: failed to reopen camera after initial test read"
        );
    }

    if (cfg.width > 0) {
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, static_cast<double>(cfg.width));
    }
    if (cfg.height > 0) {
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, static_cast<double>(cfg.height));
    }
    if (cfg.fps > 0.0) {
        cap_.set(cv::CAP_PROP_FPS, cfg.fps);
    }

#ifdef CV_CAP_PROP_BUFFERSIZE
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 2);
#endif
}

bool CameraCapture::isOpen() const
{
    return cap_.isOpened();
}

bool CameraCapture::read(Frame& frame)
{
    if (!cap_.isOpened()) {
        return false;
    }

    // Grab + retrieve in a single call
    if (!cap_.read(frame)) {
        return false;
    }

    if (frame.empty()) {
        return false;
    }

    return true;
}

} // namespace sz
