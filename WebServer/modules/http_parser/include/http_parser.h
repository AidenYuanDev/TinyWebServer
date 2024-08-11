#pragma once

#include "http_request.h"
#include <memory>
#include <string>
#include <queue>

class HttpParser {
public:
    struct ParseResult {
        bool complete;
        size_t bytes_processed;
    };

    HttpParser();
    ~HttpParser() = default;

    // 禁用拷贝
    HttpParser(const HttpParser&) = delete;
    HttpParser& operator=(const HttpParser&) = delete;

    // 允许移动
    HttpParser(HttpParser&&) noexcept = default;
    HttpParser& operator=(HttpParser&&) noexcept = default;

    ParseResult parse(const char* data, size_t len);
    bool hasCompletedRequest() const;
    HttpRequestPtr getCompletedRequest();

private:
    enum class State {
        METHOD,
        URL,
        VERSION,
        HEADER_KEY,
        HEADER_VALUE,
        BODY,
        FINISHED
    };

    State state_;
    std::unique_ptr<HttpRequest> current_request_;
    std::queue<HttpRequestPtr> completed_requests_;
    std::string current_header_key_;
    size_t content_length_;
    std::string buffer_;

    void resetParserState();
    void parseUrl(const std::string& url);
    void trim(std::string& s);
    void finalizeCurrentRequest();
};

using HttpParserPtr = std::unique_ptr<HttpParser>;
