#include "message_queue.h"

MessageQueue::MessageQueue()
    : mtx_(std::make_unique<std::mutex>()), 
      active_(true), 
      write_ready_(false) {}

MessageQueue::MessageQueue(MessageQueue&& other) noexcept
    : response_queue_(std::move(other.response_queue_)),
      mtx_(std::move(other.mtx_)),
      active_(other.active_.load()),
      write_ready_(other.write_ready_)
{
    other.active_ = false;
    other.write_ready_ = false;
}

MessageQueue& MessageQueue::operator=(MessageQueue&& other) noexcept {
    if (this != &other) {
        response_queue_ = std::move(other.response_queue_);
        mtx_ = std::move(other.mtx_);
        active_ = other.active_.load();
        write_ready_ = other.write_ready_;

        other.active_ = false;
        other.write_ready_ = false;
    }
    return *this;
}

void MessageQueue::pushResponse(HttpResponse response) {
    std::lock_guard<std::mutex> lock(*mtx_);
    response_queue_.push(std::move(response));
}

HttpResponse MessageQueue::popResponse() {
    std::lock_guard<std::mutex> lock(*mtx_);
    if (response_queue_.empty()) {
        return HttpResponse(); // 返回默认构造的响应
    }
    HttpResponse response = std::move(response_queue_.front());
    response_queue_.pop();
    return response;
}

bool MessageQueue::hasResponses() const {
    std::lock_guard<std::mutex> lock(*mtx_);
    return !response_queue_.empty();
}

void MessageQueue::setWriteReady(bool ready) {
    std::lock_guard<std::mutex> lock(*mtx_);
    write_ready_ = ready;
}

bool MessageQueue::isWriteReady() const {
    std::lock_guard<std::mutex> lock(*mtx_);
    return write_ready_;
}

bool MessageQueue::isActive() const {
    return active_;
}

void MessageQueue::deactivate() {
    active_ = false;
}
