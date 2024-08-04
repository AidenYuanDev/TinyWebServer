// src/main.cpp

#include "server.h"
#include "config_manager.h"
#include "logger.h"
#include <iostream>
#include <filesystem>
#include <stdexcept>

int main(int argc, char *argv[]) {
    try {
        // 将工作目录设置为可执行文件所在的目录
        std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

        // 初始化配置管理器
        auto& config = ConfigManager::getInstance();
        if (!config.loadConfig("server_config.yaml")) {
            std::cerr << "Failed to load configuration" << std::endl;
            return 1;
        }

        // 初始化日志系统
        Logger::getInstance().setLogLevel(config.getLogLevel());
        Logger::getInstance().setLogFile(config.getLogFile());

        LOG_INFO("Server starting...");

        // 创建并启动服务器
        Server server;
        int port = config.getPort();
        LOG_INFO("Server initialized on port %d", port);
        LOG_INFO("Using %d threads in the pool", config.getThreadPoolSize());
        LOG_INFO("Serving files from %s", config.getPublicDirectory().c_str());

        // 这里可以添加自定义的路由处理
        // 例如：server.registerHandler(HttpMethod::GET, "/api/hello", [](const HttpRequest& req) { ... });

        server.run();
    } catch (const std::exception &e) {
        LOG_FATAL("Fatal error: %s", e.what());
        return 1;
    }

    LOG_INFO("Server shutting down");
    return 0;
}
