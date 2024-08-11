#include "http_request.h"
#include <sstream>
#include <algorithm>

HttpRequest::HttpRequest()
    : method_(HttpMethod::GET), version_(HttpVersion::HTTP_1_1) {}

void HttpRequest::setMethod(HttpMethod method) {
    method_ = method;
}

void HttpRequest::setPath(const std::string& path) {
    path_ = path;
}

void HttpRequest::setQuery(const std::string& query) {
    query_ = query;
    parseQueryString();
}

void HttpRequest::setVersion(HttpVersion version) {
    version_ = version;
}

void HttpRequest::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

void HttpRequest::setBody(const std::string& body) {
    body_ = body;
}

HttpMethod HttpRequest::getMethod() const {
    return method_;
}

const std::string& HttpRequest::getPath() const {
    return path_;
}

const std::string& HttpRequest::getQuery() const {
    return query_;
}

HttpVersion HttpRequest::getVersion() const {
    return version_;
}

const Headers& HttpRequest::getHeaders() const {
    return headers_;
}

const std::string& HttpRequest::getBody() const {
    return body_;
}

std::string HttpRequest::getParameter(const std::string& key) const {
    auto it = parameters_.find(key);
    return (it != parameters_.end()) ? it->second : "";
}

bool HttpRequest::hasParameter(const std::string& key) const {
    return parameters_.find(key) != parameters_.end();
}

std::string HttpRequest::getHeader(const std::string& key) const {
    auto it = headers_.find(key);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpRequest::hasHeader(const std::string& key) const {
    return headers_.find(key) != headers_.end();
}

void HttpRequest::parseQueryString() {
    std::istringstream iss(query_);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            // Simple URL decoding
            std::replace(value.begin(), value.end(), '+', ' ');
            parameters_[key] = value;
        }
    }
}
