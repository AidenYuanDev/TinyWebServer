// config_manager.cpp
#include "config_manager.h"
#include <iostream>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::lock_guard<std::mutex> lock(configMutex);
    try {
        config = YAML::LoadFile(filename);
        configFilename = filename;
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::reloadConfig() {
    loadConfig(configFilename);
}

template<typename T>
std::optional<T> ConfigManager::getValue(const std::string& key) const {
    return getValueImpl<T>(key);
}

template<typename T>
T ConfigManager::getValue(const std::string& key, const T& defaultValue) const {
    return getValueImpl<T>(key, &defaultValue);
}

template<typename T>
T ConfigManager::getValueImpl(const std::string& key, const T* defaultValue) const {
    std::lock_guard<std::mutex> lock(configMutex);
    try {
        auto value = config[key];
        if (value.IsDefined()) {
            return value.as<T>();
        }
    } catch (const YAML::Exception& e) {
        std::cerr << "Error getting config value for key '" << key << "': " << e.what() << std::endl;
    }
    if (defaultValue) {
        return *defaultValue;
    }
    throw std::runtime_error("Config key not found and no default value provided: " + key);
}

int ConfigManager::getPort() const {
    return getValue<int>("server.port", 8080);
}

int ConfigManager::getThreadPoolSize() const {
    return getValue<int>("server.thread_pool_size", 4);
}

std::string ConfigManager::getPublicDirectory() const {
    return getValue<std::string>("server.public_directory", "./public");
}

// 实现其他特定配置项的getter方法
