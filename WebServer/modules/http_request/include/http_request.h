#pragma once
#include "http_types.h"
#include <string>
#include <memory>

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest() = default;

    // 禁用拷贝构造和赋值操作符
    HttpRequest(const HttpRequest&) = delete;
    HttpRequest& operator=(const HttpRequest&) = delete;

    // 允许移动构造和赋值
    HttpRequest(HttpRequest&&) = default;
    HttpRequest& operator=(HttpRequest&&) = default;

    // Setters
    void setMethod(HttpMethod method);
    void setPath(const std::string& path);
    void setQuery(const std::string& query);
    void setVersion(HttpVersion version);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);

    // Getters
    HttpMethod getMethod() const;
    const std::string& getPath() const;
    const std::string& getQuery() const;
    HttpVersion getVersion() const;
    const Headers& getHeaders() const;
    const std::string& getBody() const;

    // Utility methods
    std::string getParameter(const std::string& key) const;
    bool hasParameter(const std::string& key) const;
    std::string getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;

    // Parse query string
    void parseQueryString();

private:
    HttpMethod method_;
    std::string path_;
    std::string query_;
    HttpVersion version_;
    Headers headers_;
    std::string body_;
    Parameters parameters_;
};

using HttpRequestPtr = std::unique_ptr<HttpRequest>;
