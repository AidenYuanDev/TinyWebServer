// main.cpp
#include "app.h"

int main() {
    try {
        app().loadConfigFile("server_config.yaml").run();
    } catch (const std::exception& e) {
        LOG_FATAL("Fatal error: %s", e.what());
        return 1;
    }
    return 0;
}
