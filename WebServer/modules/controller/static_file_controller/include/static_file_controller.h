// static_file_controller.h
#pragma once

#include "http_types.h"
#include "http_request.h"
#include "http_response.h"
#include <string>
#include <unordered_map>

class StaticFileController {
public:
    StaticFileController(const std::string& rootDir);

    HttpResponse serveFile(const HttpRequest& req, const std::unordered_map<std::string, std::string>& params);

private:
    std::string rootDir_;
    std::unordered_map<std::string, std::string> mimeTypes_;

    void initMimeTypes();
    std::string getMimeType(const std::string& filename) const;
    bool isPathSafe(const std::string& path) const;
    HttpResponse createErrorResponse(HttpStatusCode code, const std::string& message) const;
};
