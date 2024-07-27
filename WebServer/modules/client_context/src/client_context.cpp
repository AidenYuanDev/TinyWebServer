#include "client_context.h"

ClientContext::ClientContext() : write_ready(false), active(true) {}

void ClientContext::pushResponse(const HttpResponse &response) {
    std::lock_guard<std::mutex> lock(mtx);
    response_queue.push(response);
}

bool ClientContext::hasResponses() const {
    std::lock_guard<std::mutex> lock(mtx);
    return !response_queue.empty();
}

HttpResponse ClientContext::popResponse() {
    std::lock_guard<std::mutex> lock(mtx);
    HttpResponse response = response_queue.front();
    response_queue.pop();
    return response;
}

void ClientContext::setWriteReady(bool ready) {
    std::lock_guard<std::mutex> lock(mtx);
    write_ready = ready;
}

bool ClientContext::isWriteReady() const {
    std::lock_guard<std::mutex> lock(mtx);
    return write_ready;
}

bool ClientContext::isActive() const {
    return active;
}

void ClientContext::deactivate() {
    active = false;
}
