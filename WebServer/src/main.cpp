#include "server.h"
#include "config_manager.h"
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
            throw std::runtime_error("Failed to load configuration");
        }

        // 创建并启动服务器
        Server server;
        int port = config.getPort();
        std::cout << "Server started on port " << port << std::endl;
        std::cout << "Using " << config.getThreadPoolSize() << " threads in the pool" << std::endl;
        std::cout << "Serving files from " << config.getPublicDirectory() << std::endl;

        // 这里可以添加自定义的路由处理
        // 例如：server.registerHandler(HttpMethod::GET, "/api/hello", [](const HttpRequest& req) { ... });

        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
