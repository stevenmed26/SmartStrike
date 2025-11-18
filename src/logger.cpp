#include "strikezone/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace sz {

namespace {

// Map log level to readable label
const char* levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error:   return "ERROR";
    default:                return "UNKNOWN";
    }
}

// ANSI color codes (optional cosmetic)
const char* levelToColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:   return "\033[36m"; // Cyan
    case LogLevel::Info:    return "\033[32m"; // Green
    case LogLevel::Warning: return "\033[33m"; // Yellow
    case LogLevel::Error:   return "\033[31m"; // Red
    default:                return "\033[0m";  // Reset
    }
}

std::string currentTimestamp()
{
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace

// -------------------- Logger implementation --------------------

Logger::Logger()
    : level_(LogLevel::Info)
    , useTimestamps_(true)
    , useColor_(true)
{
}

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

void Logger::setLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::enableTimestamps(bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    useTimestamps_ = enabled;
}

void Logger::enableColor(bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    useColor_ = enabled;
}

void Logger::log(LogLevel level, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Filter out messages below the current log level
    if (static_cast<int>(level) < static_cast<int>(level_)) {
        return;
    }

    std::ostringstream oss;

    if (useTimestamps_) {
        oss << "[" << currentTimestamp() << "] ";
    }

    oss << "[" << levelToString(level) << "] " << msg;

    const std::string line = oss.str();

    // Use stderr for all levels so logs are separated from any stdout output
    if (useColor_) {
        std::cerr << levelToColor(level) << line << "\033[0m" << std::endl;
    } else {
        std::cerr << line << std::endl;
    }
}

void Logger::debug(const std::string& msg)
{
    log(LogLevel::Debug, msg);
}

void Logger::info(const std::string& msg)
{
    log(LogLevel::Info, msg);
}

void Logger::warn(const std::string& msg)
{
    log(LogLevel::Warning, msg);
}

void Logger::error(const std::string& msg)
{
    log(LogLevel::Error, msg);
}

} // namespace sz
