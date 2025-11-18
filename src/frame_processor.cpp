#include "strikezone/frame_processor.hpp"
#include "strikezone/types.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

namespace sz {

namespace {

// Preprocessing constants. If you later wire in YAML config, these can come
// from model_params.yaml instead of being hardcoded here.
constexpr int   kBlurKernelSize     = 3;
constexpr double kBlurSigma         = 0.0;   // auto from kernel size
constexpr double kDenoiseStrength   = 3.0;   // for fastNlMeansDenoisingColored
constexpr double kContrastClipLimit = 2.0;   // CLAHE contrast limit
constexpr int   kTileGridSize       = 8;     // CLAHE tile grid size

// Ensure frame is in 3-channel BGR format
cv::Mat ensureBGR(const Frame& input)
{
    if (input.empty()) {
        return {};
    }

    cv::Mat bgr;

    switch (input.channels()) {
    case 1:
        cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
        break;
    case 3:
        bgr = input.clone();
        break;
    case 4:
        cv::cvtColor(input, bgr, cv::COLOR_BGRA2BGR);
        break;
    default:
        // Fallback: just clone, even if it's a strange format
        bgr = input.clone();
        break;
    }

    return bgr;
}

// Apply CLAHE (adaptive histogram equalization) on the L channel in LAB space
cv::Mat applyCLAHE(const cv::Mat& bgr)
{
    if (bgr.empty()) {
        return {};
    }

    cv::Mat lab;
    cv::cvtColor(bgr, lab, cv::COLOR_BGR2Lab);

    std::vector<cv::Mat> labChannels;
    cv::split(lab, labChannels);
    if (labChannels.empty()) {
        return bgr.clone();
    }

    // CLAHE on L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(kContrastClipLimit, cv::Size(kTileGridSize, kTileGridSize));
    clahe->apply(labChannels[0], labChannels[0]);

    cv::merge(labChannels, lab);
    cv::Mat result;
    cv::cvtColor(lab, result, cv::COLOR_Lab2BGR);
    return result;
}

} // namespace

Frame FrameProcessor::preprocess(const Frame& input) const
{
    if (input.empty()) {
        return {};
    }

    // 1. Normalize to 3-channel BGR
    cv::Mat bgr = ensureBGR(input);

    // 2. Light denoising (optional but helps with edge/contour stability)
    cv::Mat denoised;
    // Use a fast denoising method; parameters are conservative to avoid over-smoothing
    cv::fastNlMeansDenoisingColored(bgr, denoised,
                                    static_cast<float>(kDenoiseStrength),
                                    static_cast<float>(kDenoiseStrength),
                                    7, 21);

    // 3. Slight blur to remove sensor noise, while keeping edges
    cv::Mat blurred;
    cv::GaussianBlur(denoised, blurred,
                     cv::Size(kBlurKernelSize, kBlurKernelSize),
                     kBlurSigma);

    // 4. Contrast enhancement with CLAHE in LAB space
    cv::Mat enhanced = applyCLAHE(blurred);

    // 5. Return the processed frame for downstream modules (plate/batter/ball)
    return enhanced;
}

} // namespace sz
