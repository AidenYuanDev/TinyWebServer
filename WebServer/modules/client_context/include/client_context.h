// modules/client_context/include/client_context.h

#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include "http_parser.h"
#include "http_response.h"
#include "logger.h"

class ClientContext {
public:
    ClientContext();
    void pushResponse(const HttpResponse &response);
    bool hasResponses() const;
    HttpResponse popResponse();
    void setWriteReady(bool ready);
    bool isWriteReady() const;
    bool isActive() const;
    void deactivate();
    HttpParser parser;

private:
    std::queue<HttpResponse> response_queue;
    bool write_ready;
    mutable std::mutex mtx;
    std::atomic<bool> active;
};
