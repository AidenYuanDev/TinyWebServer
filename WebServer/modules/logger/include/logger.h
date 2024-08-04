// modules/logger/include/logger.h

#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>
#include <iostream>

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

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);

    template<typename... Args>
    void debug(const char* format, Args... args) {
        log(LogLevel::DEBUG, format, args...);
    }

    template<typename... Args>
    void info(const char* format, Args... args) {
        log(LogLevel::INFO, format, args...);
    }

    template<typename... Args>
    void warning(const char* format, Args... args) {
        log(LogLevel::WARNING, format, args...);
    }

    template<typename... Args>
    void error(const char* format, Args... args) {
        log(LogLevel::ERROR, format, args...);
    }

    template<typename... Args>
    void fatal(const char* format, Args... args) {
        log(LogLevel::FATAL, format, args...);
    }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        if (level >= currentLevel) {
            std::string message = formatMessage(format, args...);
            writeLog(level, message);
        }
    }

    template<typename... Args>
    std::string formatMessage(const char* format, Args... args) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        return std::string(buffer);
    }

    void writeLog(LogLevel level, const std::string& message);
    std::string getCurrentTimestamp();
    std::string getLevelString(LogLevel level);

    LogLevel currentLevel;
    std::ofstream logFile;
    std::mutex logMutex;
};

#define LOG_DEBUG(...) Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...)  Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARN(...)  Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().error(__VA_ARGS__)
#define LOG_FATAL(...) Logger::getInstance().fatal(__VA_ARGS__)
