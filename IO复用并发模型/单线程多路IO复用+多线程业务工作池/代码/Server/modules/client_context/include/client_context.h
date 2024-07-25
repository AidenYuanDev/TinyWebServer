#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
using namespace std;

class ClientContext {
public:
    ClientContext() : active(true) {}
    void pushMessage(const string &msg);
    bool hasMessages() const;
    string popMessage();
    void setWriteReady(bool ready);
    bool isWriteReady() const;
    bool isActive() const;
    void deactivate();

private:
    queue<string> send_queue;
    bool write_ready = false;
    mutable mutex mtx;
    atomic<bool> active;
};
