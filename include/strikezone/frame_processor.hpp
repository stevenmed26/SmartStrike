#pragma once

#include "strikezone/types.hpp"

namespace sz {

class FrameProcessor {
public:
    FrameProcessor() = default;

    // Apply denoising, blur, and contrast enhancement
    Frame preprocess(const Frame& input) const;
};

} // namespace sz

