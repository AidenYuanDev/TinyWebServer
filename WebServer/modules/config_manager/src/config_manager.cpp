// modules/config_manager/src/config_manager.cpp

#include "config_manager.h"

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filename) {
    std::lock_guard<std::mutex> lock(configMutex);
    try {
        config = YAML::LoadFile(filename);
        configFilename = filename;
        LOG_INFO("Configuration loaded successfully from %s", filename.c_str());
        return true;
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Error loading config file %s: %s", filename.c_str(), e.what());
        return false;
    }
}

void ConfigManager::reloadConfig() {
    LOG_INFO("Reloading configuration from %s", configFilename.c_str());
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
            LOG_DEBUG("Retrieved config value for key '%s'", key.c_str());
            return value.as<T>();
        }
    } catch (const YAML::Exception& e) {
        LOG_ERROR("Error getting config value for key '%s': %s", key.c_str(), e.what());
    }
    if (defaultValue) {
        LOG_WARN("Using default value for config key '%s'", key.c_str());
        return *defaultValue;
    }
    LOG_ERROR("Config key not found and no default value provided: %s", key.c_str());
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

LogLevel ConfigManager::getLogLevel() const {
    std::string logLevelStr = getValue<std::string>("logging.level", "INFO");
    if (logLevelStr == "DEBUG") return LogLevel::DEBUG;
    if (logLevelStr == "INFO") return LogLevel::INFO;
    if (logLevelStr == "WARNING") return LogLevel::WARNING;
    if (logLevelStr == "ERROR") return LogLevel::ERROR;
    if (logLevelStr == "FATAL") return LogLevel::FATAL;
    LOG_WARN("Unknown log level '%s', defaulting to INFO", logLevelStr.c_str());
    return LogLevel::INFO;
}

std::string ConfigManager::getLogFile() const {
    return getValue<std::string>("logging.file", "server.log");
}

// Explicit template instantiations
template std::optional<int> ConfigManager::getValue<int>(const std::string&) const;
template std::optional<std::string> ConfigManager::getValue<std::string>(const std::string&) const;
template std::optional<bool> ConfigManager::getValue<bool>(const std::string&) const;
template std::optional<double> ConfigManager::getValue<double>(const std::string&) const;

template int ConfigManager::getValue<int>(const std::string&, const int&) const;
template std::string ConfigManager::getValue<std::string>(const std::string&, const std::string&) const;
template bool ConfigManager::getValue<bool>(const std::string&, const bool&) const;
template double ConfigManager::getValue<double>(const std::string&, const double&) const;
