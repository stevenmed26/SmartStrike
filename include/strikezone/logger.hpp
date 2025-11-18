#pragma once

#include <mutex>
#include <string>

namespace sz {

enum class LogLevel {
    Debug = 0,
    Info  = 1,
    Warning = 2,
    Error = 3
};

class Logger {
public:
    static Logger& instance();

    void setLevel(LogLevel level);
    [[nodiscard]] LogLevel level() const;

    void enableTimestamps(bool enabled);
    void enableColor(bool enabled);

    void log(LogLevel level, const std::string& msg);

    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    Logger();

    mutable std::mutex mutex_;
    LogLevel level_;
    bool useTimestamps_;
    bool useColor_;
};

} // namespace sz
