// modules/config_manager/include/config_manager.h

#pragma once

#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <mutex>
#include "logger.h"

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filename);
    void reloadConfig();

    template<typename T>
    std::optional<T> getValue(const std::string& key) const;

    template<typename T>
    T getValue(const std::string& key, const T& defaultValue) const;

    // 特定配置项的getter方法
    int getPort() const;
    int getThreadPoolSize() const;
    std::string getPublicDirectory() const;
    LogLevel getLogLevel() const;
    std::string getLogFile() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    YAML::Node config;
    mutable std::mutex configMutex;
    std::string configFilename;

    template<typename T>
    T getValueImpl(const std::string& key, const T* defaultValue = nullptr) const;
};
