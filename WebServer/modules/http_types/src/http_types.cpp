#include "http_types.h"
#include <unordered_map>

std::optional<std::string> method_to_string(HttpMethod method) {
    static const std::unordered_map<HttpMethod, std::string> method_map = {
        {HttpMethod::GET, "GET"},
        {HttpMethod::POST, "POST"},
        {HttpMethod::PUT, "PUT"},
        {HttpMethod::DELETE, "DELETE"},
        {HttpMethod::HEAD, "HEAD"},
        {HttpMethod::OPTIONS, "OPTIONS"},
        {HttpMethod::PATCH, "PATCH"},
        {HttpMethod::TRACE, "TRACE"},
        {HttpMethod::CONNECT, "CONNECT"}
    };

    auto it = method_map.find(method);
    if (it != method_map.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::variant<HttpMethod, HttpError> string_to_method(const std::string& method_str) {
    static const std::unordered_map<std::string, HttpMethod> method_map = {
        {"GET", HttpMethod::GET},
        {"POST", HttpMethod::POST},
        {"PUT", HttpMethod::PUT},
        {"DELETE", HttpMethod::DELETE},
        {"HEAD", HttpMethod::HEAD},
        {"OPTIONS", HttpMethod::OPTIONS},
        {"PATCH", HttpMethod::PATCH},
        {"TRACE", HttpMethod::TRACE},
        {"CONNECT", HttpMethod::CONNECT}
    };

    auto it = method_map.find(method_str);
    if (it != method_map.end()) {
        return it->second;
    }
    return HttpError{HttpStatusCode::BAD_REQUEST, "Invalid HTTP method string"};
}

std::optional<std::string> version_to_string(HttpVersion version) {
    switch (version) {
        case HttpVersion::HTTP_1_0: return "HTTP/1.0";
        case HttpVersion::HTTP_1_1: return "HTTP/1.1";
        case HttpVersion::HTTP_2_0: return "HTTP/2.0";
        default: return std::nullopt;
    }
}

std::variant<HttpVersion, HttpError> string_to_version(const std::string& version_str) {
    if (version_str == "HTTP/1.0") return HttpVersion::HTTP_1_0;
    if (version_str == "HTTP/1.1") return HttpVersion::HTTP_1_1;
    if (version_str == "HTTP/2.0") return HttpVersion::HTTP_2_0;
    return HttpError{HttpStatusCode::BAD_REQUEST, "Invalid HTTP version string"};
}

std::optional<std::string> status_code_to_string(HttpStatusCode status_code) {
    static const std::unordered_map<HttpStatusCode, std::string> status_map = {
        {HttpStatusCode::OK, "200 OK"},
        {HttpStatusCode::CREATED, "201 Created"},
        {HttpStatusCode::ACCEPTED, "202 Accepted"},
        {HttpStatusCode::NO_CONTENT, "204 No Content"},
        {HttpStatusCode::MOVED_PERMANENTLY, "301 Moved Permanently"},
        {HttpStatusCode::FOUND, "302 Found"},
        {HttpStatusCode::BAD_REQUEST, "400 Bad Request"},
        {HttpStatusCode::UNAUTHORIZED, "401 Unauthorized"},
        {HttpStatusCode::FORBIDDEN, "403 Forbidden"},
        {HttpStatusCode::NOT_FOUND, "404 Not Found"},
        {HttpStatusCode::METHOD_NOT_ALLOWED, "405 Method Not Allowed"},
        {HttpStatusCode::INTERNAL_SERVER_ERROR, "500 Internal Server Error"},
        {HttpStatusCode::NOT_IMPLEMENTED, "501 Not Implemented"},
        {HttpStatusCode::BAD_GATEWAY, "502 Bad Gateway"},
        {HttpStatusCode::SERVICE_UNAVAILABLE, "503 Service Unavailable"}
    };

    auto it = status_map.find(status_code);
    if (it != status_map.end()) {
        return it->second;
    }
    return std::nullopt;
}
