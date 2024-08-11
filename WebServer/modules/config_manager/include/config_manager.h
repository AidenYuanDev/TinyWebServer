// config_manager.h
#pragma once

#include <yaml-cpp/yaml.h>
#include <string>
#include "logger.h"

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filename);
    void reloadConfig();

    // 特定配置项的getter方法
    int getPort() const;
    int getThreadPoolSize() const;
    std::string getPublicDirectory() const;
    LogLevel getLogLevel() const;
    std::string getLogFile() const;

private:
    YAML::Node config;
    std::string config_filename;
};

#define config() ConfigManager::getInstance() 
