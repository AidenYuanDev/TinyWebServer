// modules/client_context/src/client_context.cpp

#include "client_context.h"
#include <config_manager.h>

ClientContext::ClientContext() : write_ready(false), active(true) {
    LOG_DEBUG("ClientContext created");
}

void ClientContext::pushResponse(const HttpResponse &response) {
    std::lock_guard<std::mutex> lock(mtx);
    response_queue.push(response);
    LOG_DEBUG("Response pushed to client context queue, queue size: %zu", response_queue.size());
}

bool ClientContext::hasResponses() const {
    std::lock_guard<std::mutex> lock(mtx);
    return !response_queue.empty();
}

HttpResponse ClientContext::popResponse() {
    std::lock_guard<std::mutex> lock(mtx);
    if (response_queue.empty()) {
        LOG_WARN("Attempt to pop response from empty queue");
        return HttpResponse(); // Return default constructed response
    }
    HttpResponse response = response_queue.front();
    response_queue.pop();
    LOG_DEBUG("Response popped from client context queue, remaining size: %zu", response_queue.size());
    return response;
}

void ClientContext::setWriteReady(bool ready) {
    std::lock_guard<std::mutex> lock(mtx);
    write_ready = ready;
    LOG_DEBUG("Client context write ready state set to: %s", ready ? "true" : "false");
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
    LOG_INFO("Client context deactivated");
}
