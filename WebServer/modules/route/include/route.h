// route.h
#pragma once

#include "http_types.h"
#include "http_request.h"
#include "http_response.h"
#include <string>
#include <functional>
#include <vector>
#include <unordered_map>


using RequestHandler = std::function<HttpResponse(const HttpRequest&, const std::unordered_map<std::string, std::string>&)>;


class Route {
public:
    Route(std::string path, HttpMethod method, RequestHandler handler);
    ~Route() = default;

    // 禁用拷贝构造和赋值操作符
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;

    // 允许移动构造和赋值
    Route(Route&&) noexcept = default;
    Route& operator=(Route&&) noexcept = default;

    bool matches(const std::vector<std::string>& pathSegments, HttpMethod method) const;
    std::unordered_map<std::string, std::string> extractParams(const std::vector<std::string>& pathSegments) const;
    const RequestHandler& getHandler() const;

    const std::string& getPath() const;
    HttpMethod getMethod() const;

private:
    struct PathSegment {
        std::string value;
        bool isParameter;
    };

    std::string path_;
    HttpMethod method_;
    RequestHandler handler_;
    std::vector<PathSegment> segments_;

    void parsePathSegments();
};
