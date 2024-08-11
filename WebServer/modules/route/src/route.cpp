// route.cpp
#include "route.h"

#include <sstream>

Route::Route(std::string path, HttpMethod method, RequestHandler handler) : path_(std::move(path)), method_(method), handler_(std::move(handler)) { parsePathSegments(); }

bool Route::matches(const std::vector<std::string> &pathSegments, HttpMethod method) const {
    if (method_ != method || pathSegments.size() != segments_.size()) {
        return false;
    }
    for (size_t i = 0; i < segments_.size(); ++i) {
        if (!segments_[i].isParameter && segments_[i].value != pathSegments[i]) {
            return false;
        }
    }
    return true;
}

std::unordered_map<std::string, std::string> Route::extractParams(const std::vector<std::string> &pathSegments) const {
    std::unordered_map<std::string, std::string> params;
    for (size_t i = 0; i < segments_.size(); ++i) {
        if (segments_[i].isParameter) {
            params[segments_[i].value] = pathSegments[i];
        }
    }
    return params;
}

const RequestHandler &Route::getHandler() const { return handler_; }

const std::string &Route::getPath() const { return path_; }

HttpMethod Route::getMethod() const { return method_; }

void Route::parsePathSegments() {
    std::istringstream iss(path_);
    std::string segment;
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) {
            bool isParam = segment[0] == ':';
            segments_.push_back({isParam ? segment.substr(1) : segment, isParam});
        }
    }
}
