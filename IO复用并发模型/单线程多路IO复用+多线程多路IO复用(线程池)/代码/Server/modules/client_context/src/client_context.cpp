#include "client_context.h"

// ClientContext implementation
void ClientContext::pushMessage(const string &msg) {
    lock_guard<mutex> lock(mtx);
    send_queue.push(msg);
}

bool ClientContext::hasMessages() const {
    lock_guard<mutex> lock(mtx);
    return !send_queue.empty();
}
string ClientContext::popMessage() {
    lock_guard<mutex> lock(mtx);
    string msg = send_queue.front();
    send_queue.pop();
    return msg;
}
void ClientContext::setWriteReady(bool ready) {
    lock_guard<mutex> lock(mtx);
    write_ready = ready;
}
bool ClientContext::isWriteReady() const {
    lock_guard<mutex> lock(mtx);
    return write_ready;
}
bool ClientContext::isActive() const { return active; }
void ClientContext::deactivate() { active = false; }
