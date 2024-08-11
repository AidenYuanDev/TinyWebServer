#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <format>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    Logger& setLogLevel(LogLevel level);
    Logger& setLogFile(const std::string& filename);

    template<typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::INFO, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warning(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::WARNING, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::ERROR, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::FATAL, fmt, std::forward<Args>(args)...);
    }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (level >= currentLevel) {
            std::string message = std::format(fmt, std::forward<Args>(args)...);
            writeLog(level, message);
        }
    }

    void writeLog(LogLevel level, const std::string& message);
    std::string getCurrentTimestamp();
    std::string getLevelString(LogLevel level);

    LogLevel currentLevel;
    std::ofstream logFile;
    std::mutex logMutex;
};

#define logger() Logger::getInstance()

#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...)  Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...)  Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_FATAL(...) Logger::getInstance().fatal(__VA_ARGS__)
