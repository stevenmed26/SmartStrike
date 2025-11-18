#pragma once

#include "strikezone/types.hpp"

#include <opencv2/videoio.hpp>

namespace sz {

struct CameraConfig; // forward declaration from config.hpp

class CameraCapture {
public:
    explicit CameraCapture(const CameraConfig& cfg);

    bool isOpen() const;
    bool read(Frame& frame);

private:
    cv::VideoCapture cap_;
};

} // namespace sz

