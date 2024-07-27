// http_response.cpp
#include "http_response.h"
#include <sstream>

HttpResponse::HttpResponse(int code, std::string_view message)
    : status_code(code), status_message(message) {}

void HttpResponse::setBody(std::string_view content) {
    body.assign(content.begin(), content.end());
    headers["Content-Length"] = std::to_string(body.size());
}

void HttpResponse::setBody(const std::vector<char>& content) {
    body = content;
    headers["Content-Length"] = std::to_string(body.size());
}

std::string HttpResponse::toString() const {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    for (const auto& [key, value] : headers) {
        response << key << ": " << value << "\r\n";
    }
    response << "\r\n";
    return response.str();
}

HttpResponse HttpResponseFactory::createNotFoundResponse() {
    HttpResponse response(404, "Not Found");
    response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
    response.headers["Content-Type"] = "text/html";
    return response;
}

HttpResponse HttpResponseFactory::createOkResponse(const std::vector<char>& body, std::string_view contentType) {
    HttpResponse response(200, "OK");
    response.setBody(body);
    response.headers["Content-Type"] = std::string(contentType);
    return response;
}

HttpResponse HttpResponseFactory::createRedirectResponse(std::string_view location) {
    HttpResponse response(302, "Found");
    response.headers["Location"] = std::string(location);
    return response;
}

HttpResponse HttpResponseFactory::createServerErrorResponse() {
    HttpResponse response(500, "Internal Server Error");
    response.setBody("<html><body><h1>500 Internal Server Error</h1></body></html>");
    response.headers["Content-Type"] = "text/html";
    return response;
}
