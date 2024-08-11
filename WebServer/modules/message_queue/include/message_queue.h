#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <memory>
#include "http_response.h"

class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue() = default;

    // 禁用拷贝
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    // 允许移动
    MessageQueue(MessageQueue&& other) noexcept;
    MessageQueue& operator=(MessageQueue&& other) noexcept;

    // 响应处理
    void pushResponse(HttpResponse response);
    HttpResponse popResponse();
    bool hasResponses() const;

    // 状态管理
    void setWriteReady(bool ready);
    bool isWriteReady() const;
    bool isActive() const;
    void deactivate();

private:
    std::queue<HttpResponse> response_queue_;
    std::unique_ptr<std::mutex> mtx_;
    std::atomic<bool> active_;
    bool write_ready_;
};
