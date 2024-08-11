#include "http_response.h"
#include <sstream>

HttpResponse::HttpResponse()
    : statusCode_(HttpStatusCode::OK), 
      version_(HttpVersion::HTTP_1_1) {}

HttpResponse& HttpResponse::setStatusCode(HttpStatusCode code) {
    statusCode_ = code;
    return *this;
}

HttpResponse& HttpResponse::setVersion(HttpVersion version) {
    version_ = version;
    return *this;
}

HttpResponse& HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

HttpResponse& HttpResponse::setBody(const std::string& body) {
    body_ = body;
    updateContentLength();
    return *this;
}

HttpResponse& HttpResponse::appendBody(const std::string& str) {
    body_ += str;
    updateContentLength();
    return *this;
}

HttpStatusCode HttpResponse::getStatusCode() const {
    return statusCode_;
}

HttpVersion HttpResponse::getVersion() const {
    return version_;
}

const Headers& HttpResponse::getHeaders() const {
    return headers_;
}

const std::string& HttpResponse::getBody() const {
    return body_;
}

std::string HttpResponse::getHeader(const std::string& key) const {
    auto it = headers_.find(key);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpResponse::hasHeader(const std::string& key) const {
    return headers_.find(key) != headers_.end();
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    oss << (version_ == HttpVersion::HTTP_1_1 ? "HTTP/1.1 " : "HTTP/1.0 ")
        << static_cast<int>(statusCode_) << " " << getStatusMessage(statusCode_) << "\r\n";

    for (const auto& [key, value] : headers_) {
        oss << key << ": " << value << "\r\n";
    }
    oss << "\r\n";
    oss << body_;
    return oss.str();
}

HttpResponse HttpResponse::newHttpResponse() {
    return HttpResponse();
}

HttpResponse HttpResponse::makeOkResponse() {
    HttpResponse resp;
    resp.setStatusCode(HttpStatusCode::OK)
        .setHeader("Content-Type", "text/plain")
        .setBody("OK");
    return resp;
}

HttpResponse HttpResponse::makeNotFoundResponse() {
    HttpResponse resp;
    resp.setStatusCode(HttpStatusCode::NOT_FOUND)
        .setHeader("Content-Type", "text/plain")
        .setBody("404 Not Found");
    return resp;
}

HttpResponse HttpResponse::makeInternalServerErrorResponse() {
    HttpResponse resp;
    resp.setStatusCode(HttpStatusCode::INTERNAL_SERVER_ERROR)
        .setHeader("Content-Type", "text/plain")
        .setBody("500 Internal Server Error");
    return resp;
}

std::string HttpResponse::getStatusMessage(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::CREATED: return "Created";
        case HttpStatusCode::ACCEPTED: return "Accepted";
        case HttpStatusCode::NO_CONTENT: return "No Content";
        case HttpStatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatusCode::FOUND: return "Found";
        case HttpStatusCode::BAD_REQUEST: return "Bad Request";
        case HttpStatusCode::UNAUTHORIZED: return "Unauthorized";
        case HttpStatusCode::FORBIDDEN: return "Forbidden";
        case HttpStatusCode::NOT_FOUND: return "Not Found";
        case HttpStatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatusCode::BAD_GATEWAY: return "Bad Gateway";
        case HttpStatusCode::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown Status";
    }
}

void HttpResponse::updateContentLength() {
    setHeader("Content-Length", std::to_string(body_.length()));
}
