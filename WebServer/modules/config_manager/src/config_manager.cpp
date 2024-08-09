#include "config_manager.h"

#include <string>

ConfigManager &ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string &filename) {
    try {
        config = YAML::LoadFile(filename);
        config_filename = filename;
        LOG_INFO("Configuration loaded successfully from %s", filename);
        return true;
    } catch (const YAML::Exception &e) {
        LOG_ERROR("Error loading config file %s: %s", filename, e.what());
        return false;
    }
}

void ConfigManager::reloadConfig() {
    LOG_INFO("Reloading configuration from %s", config_filename.c_str());
    loadConfig(config_filename);
}

int ConfigManager::getPort() const {
    try {
        return config["server"]["port"].as<int>();
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("Error getting double value for key server port: " + std::string(e.what()));
    }
}

int ConfigManager::getThreadPoolSize() const {
    try {
        return config["server"]["thread_pool_size"].as<int>();
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("Error getting double value for key server thread_pool_size: " + std::string(e.what()));
    }
}

std::string ConfigManager::getPublicDirectory() const {
    try {
        return config["server"]["directory"].as<std::string>();
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("Error getting double value for key server directory: " + std::string(e.what()));
    }
}

LogLevel ConfigManager::getLogLevel() const {
    try {
        std::string level = config["logger"]["level"].as<std::string>();
        if (level == "DEBUG") return LogLevel::DEBUG;
        if (level == "INFO") return LogLevel::INFO;
        if (level == "WARNING") return LogLevel::WARNING;
        if (level == "ERROR") return LogLevel::ERROR;
        if (level == "FATAL") return LogLevel::FATAL;
        LOG_WARN("Unknown log level '%s', defaulting to INFO", level.c_str());
        return LogLevel::INFO;
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("Error getting double value for key LogLevel: " + std::string(e.what()));
    }
}

std::string ConfigManager::getLogFile() const {
    try {
        return config["logger"]["name"].as<std::string>();
    } catch (const YAML::Exception &e) {
        throw std::runtime_error("Error getting double value for key logger name: " + std::string(e.what()));
    }
}
