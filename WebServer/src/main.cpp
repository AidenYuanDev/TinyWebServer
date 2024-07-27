#include "server.h"
#include <iostream>
#include <filesystem>

const int PORT = 8080;

int main(int argc, char *argv[]) {
    // 将工作目录设置为可执行文件所在的目录
    std::filesystem::current_path(std::filesystem::path(argv[0]).parent_path());

    try {
        Server server(PORT);
        std::cout << "Server started on port " << PORT << std::endl;
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
