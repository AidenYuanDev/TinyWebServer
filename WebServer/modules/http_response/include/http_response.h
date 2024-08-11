#pragma once

#include "http_types.h"
#include <string>

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse() = default;

    // 禁用拷贝构造和赋值操作符
    HttpResponse(const HttpResponse&) = delete;
    HttpResponse& operator=(const HttpResponse&) = delete;

    // 允许移动构造和赋值
    HttpResponse(HttpResponse&&) noexcept = default;
    HttpResponse& operator=(HttpResponse&&) noexcept = default;

    // 构建器方法
    HttpResponse& setStatusCode(HttpStatusCode code);
    HttpResponse& setVersion(HttpVersion version);
    HttpResponse& setHeader(const std::string& key, const std::string& value);
    HttpResponse& setBody(const std::string& body);
    HttpResponse& appendBody(const std::string& str);

    // Getters
    HttpStatusCode getStatusCode() const;
    HttpVersion getVersion() const;
    const Headers& getHeaders() const;
    const std::string& getBody() const;

    // Utility methods
    std::string getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;

    // 序列化响应为字符串
    std::string toString() const;

    // 快捷方法创建常见响应类型
    static HttpResponse newHttpResponse();
    static HttpResponse makeOkResponse();
    static HttpResponse makeNotFoundResponse();
    static HttpResponse makeInternalServerErrorResponse();

    // 获取标准状态码描述
    static std::string getStatusMessage(HttpStatusCode code);

private:
    HttpStatusCode statusCode_;
    HttpVersion version_;
    Headers headers_;
    std::string body_;

    void updateContentLength();
};
