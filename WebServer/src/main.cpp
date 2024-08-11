// main.cpp
#include "app.h"

int main(int argc, char* argv[]) {
    try {
        app().loadConfigFile("server_config.yaml").run();
    } catch (const std::exception& e) {
        LOG_FATAL("Fatal error: %s", e.what());
        return 1;
    }
    return 0;
}
