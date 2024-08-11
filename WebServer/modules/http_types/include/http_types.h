#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <variant>

// HTTP 方法枚举
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    PATCH,
    TRACE,
    CONNECT
};

// HTTP 版本枚举
enum class HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0
};

// HTTP 状态码枚举
enum class HttpStatusCode {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503
};

// HTTP 头部类型
using Headers = std::unordered_map<std::string, std::string>;

// HTTP 参数类型
using Parameters = std::unordered_map<std::string, std::string>;

// 错误类型
struct HttpError {
    HttpStatusCode status_code;
    std::string message;
};

// 将 HttpMethod 枚举转换为字符串
std::optional<std::string> method_to_string(HttpMethod method);

// 将字符串转换为 HttpMethod 枚举
std::variant<HttpMethod, HttpError> string_to_method(const std::string& method_str);

// 将 HttpVersion 枚举转换为字符串
std::optional<std::string> version_to_string(HttpVersion version);

// 将字符串转换为 HttpVersion 枚举
std::variant<HttpVersion, HttpError> string_to_version(const std::string& version_str);

// 获取状态码对应的描述
std::optional<std::string> status_code_to_string(HttpStatusCode status_code);
