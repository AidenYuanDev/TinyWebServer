// app.h
#pragma once

#include <string>
#include "server.h"

class App {
public:
    static App& getInstance();
    
    App& loadConfigFile(const std::string& fileName);
    void run();

private:
    App() = default;
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    std::unique_ptr<Server> server;
    void initializeComponents();
};

#define app() App::getInstance()
