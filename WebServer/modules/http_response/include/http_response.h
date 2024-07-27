// http_response.h
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class HttpResponse {
public:
    int status_code = 200;
    std::string status_message = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::vector<char> body;

    HttpResponse() = default;
    HttpResponse(int code, std::string_view message);

    void setBody(std::string_view content);
    void setBody(const std::vector<char>& content);
    std::string toString() const;
};

class HttpResponseFactory {
public:
    static HttpResponse createNotFoundResponse();
    static HttpResponse createOkResponse(const std::vector<char>& body, std::string_view contentType);
    static HttpResponse createRedirectResponse(std::string_view location);
    static HttpResponse createServerErrorResponse();
};
