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
    //   ./strikezone_realtime              # uses "./config" as default config directory
    //   ./strikezone_realtime /path/to/config
    //
    std::string configRoot = "config";
    if (argc >= 2) {
        configRoot = argv[1];
    }

    try {
        // -------------------- Initialize logging --------------------
        Logger::instance().setLevel(LogLevel::Info);
        Logger::instance().enableTimestamps(true);
        Logger::instance().enableColor(true);

        Logger::instance().info("Strike Zone Detector - Realtime Mode");
        Logger::instance().info("Using config directory: " + configRoot);

        // -------------------- Load configuration --------------------
        Config cfg(configRoot);

        // -------------------- Create and run pipeline --------------------
        Pipeline pipeline(cfg);
        pipeline.runRealtime();

        Logger::instance().info("Strike Zone Detector - Realtime Mode terminated cleanly");
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "[FATAL] " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "[FATAL] Unknown exception in main_realtime" << std::endl;
        return 2;
    }
}
