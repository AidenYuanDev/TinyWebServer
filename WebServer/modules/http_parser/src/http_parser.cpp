#include "http_parser.h"
#include <algorithm>
#include <cctype>

HttpParser::HttpParser() 
    : state_(State::METHOD), 
      current_request_(std::make_unique<HttpRequest>()), 
      content_length_(0) {}

HttpParser::ParseResult HttpParser::parse(const char* data, size_t len) {
    buffer_.append(data, len);
    size_t total_processed = 0;

    while (!buffer_.empty()) {
        const char* start = buffer_.c_str();
        const char* end = start + buffer_.length();
        const char* current = start;

        while (current < end) {
            switch (state_) {
                case State::METHOD:
                    if (*current == ' ') {
                        auto result = string_to_method(std::string(start, current));
                        if (auto method = std::get_if<HttpMethod>(&result)) {
                            current_request_->setMethod(*method);
                            state_ = State::URL;
                            start = current + 1;
                        } else {
                            // Handle error
                            return {false, total_processed};
                        }
                    }
                    break;
                case State::URL:
                    if (*current == ' ') {
                        parseUrl(std::string(start, current));
                        state_ = State::VERSION;
                        start = current + 1;
                    }
                    break;
                case State::VERSION:
                    if (*current == '\r') {
                        auto result = string_to_version(std::string(start, current));
                        if (auto version = std::get_if<HttpVersion>(&result)) {
                            current_request_->setVersion(*version);
                        } else {
                            // Handle error
                            return {false, total_processed};
                        }
                    } else if (*current == '\n') {
                        state_ = State::HEADER_KEY;
                        start = current + 1;
                    }
                    break;
                case State::HEADER_KEY:
                    if (*current == ':') {
                        current_header_key_ = std::string(start, current);
                        state_ = State::HEADER_VALUE;
                        start = current + 1;
                    } else if (*current == '\r') {
                        // Empty line, end of headers
                    } else if (*current == '\n') {
                        if (current_request_->hasHeader("Content-Length")) {
                            content_length_ = std::stoul(current_request_->getHeader("Content-Length"));
                            state_ = State::BODY;
                        } else {
                            finalizeCurrentRequest();
                        }
                        start = current + 1;
                    }
                    break;
                case State::HEADER_VALUE:
                    if (*current == '\r') {
                        std::string value(start, current);
                        trim(value);
                        current_request_->setHeader(current_header_key_, value);
                    } else if (*current == '\n') {
                        state_ = State::HEADER_KEY;
                        start = current + 1;
                    }
                    break;
                case State::BODY:
                    {
                        size_t remaining = content_length_ - current_request_->getBody().length();
                        size_t to_read = std::min(remaining, static_cast<size_t>(end - current));
                        current_request_->setBody(current_request_->getBody() + std::string(current, to_read));
                        current += to_read - 1; // -1 because the loop will increment current
                        if (current_request_->getBody().length() == content_length_) {
                            finalizeCurrentRequest();
                        }
                    }
                    break;
                case State::FINISHED:
                    // This should not happen, as we reset the state after finalizing a request
                    break;
            }
            ++current;
        }

        size_t processed = current - start;
        total_processed += processed;
        buffer_ = buffer_.substr(processed);

        if (state_ != State::FINISHED) {
            // We need more data to complete the current request
            break;
        }
    }

    return {!completed_requests_.empty(), total_processed};
}

bool HttpParser::hasCompletedRequest() const {
    return !completed_requests_.empty();
}

HttpRequestPtr HttpParser::getCompletedRequest() {
    if (completed_requests_.empty()) {
        return nullptr;
    }
    auto request = std::move(completed_requests_.front());
    completed_requests_.pop();
    return request;
}

void HttpParser::resetParserState() {
    state_ = State::METHOD;
    current_request_ = std::make_unique<HttpRequest>();
    current_header_key_.clear();
    content_length_ = 0;
}

void HttpParser::parseUrl(const std::string& url) {
    auto pos = url.find('?');
    if (pos != std::string::npos) {
        current_request_->setPath(url.substr(0, pos));
        current_request_->setQuery(url.substr(pos + 1));
    } else {
        current_request_->setPath(url);
    }
}

void HttpParser::trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void HttpParser::finalizeCurrentRequest() {
    completed_requests_.push(std::move(current_request_));
    resetParserState();
}
