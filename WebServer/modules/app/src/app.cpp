// app.cpp
#include "app.h"
#include "config_manager.h"
#include "logger.h"

App& App::getInstance() {
    static App instance;
    return instance;
}

App& App::loadConfigFile(const std::string& fileName) {
    config().loadConfig(fileName);
    logger().setLogLevel(config().getLogLevel())
            .setLogFile(config().getLogFile());
    LOG_INFO("Configuration loaded from %s", fileName.c_str());
    return *this;
}

void App::initializeComponents() {
    int port = config().getPort();
    std::string publicDir = config().getPublicDirectory();
    int threadPoolSize = config().getThreadPoolSize();

    server = std::make_unique<Server>(port, publicDir, threadPoolSize);

    // 注册路由

    // 可以添加更多的路由注册
}

void App::run() {
    LOG_INFO("Initializing application components...");
    initializeComponents();
    
    LOG_INFO("Starting server on port %d", config().getPort());
    server->run();
}
