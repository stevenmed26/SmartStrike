#include "strikezone/config.hpp"
#include "strikezone/pipeline.hpp"
#include "strikezone/logger.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
    using namespace sz;

    // -------------------- Parse arguments --------------------
    // Usage:
    //   ./strikezone_offline <video_path>                 # uses "./config" as default config directory
    //   ./strikezone_offline <video_path> <config_dir>    # custom config directory
    //
    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <video_path> [config_dir]\n\n"
                  << "Examples:\n"
                  << "  " << argv[0] << " data/sample_video.mp4\n"
                  << "  " << argv[0] << " data/sample_video.mp4 config\n";
        return 1;
    }

    const std::string videoPath  = argv[1];
    std::string configRoot       = "config";

    if (argc >= 3) {
        configRoot = argv[2];
    }

    try {
        // -------------------- Initialize logging --------------------
        Logger::instance().setLevel(LogLevel::Info);
        Logger::instance().enableTimestamps(true);
        Logger::instance().enableColor(true);

        Logger::instance().info("Strike Zone Detector - Offline Mode");
        Logger::instance().info("Using config directory: " + configRoot);
        Logger::instance().info("Video path: " + videoPath);

        // -------------------- Load configuration --------------------
        Config cfg(configRoot);

        // -------------------- Create and run pipeline --------------------
        Pipeline pipeline(cfg);
        pipeline.runOffline(videoPath);

        Logger::instance().info("Strike Zone Detector - Offline Mode terminated cleanly");
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "[FATAL] " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "[FATAL] Unknown exception in main_offline" << std::endl;
        return 2;
    }
}
