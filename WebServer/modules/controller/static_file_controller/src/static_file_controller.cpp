// static_file_controller.cpp
#include "static_file_controller.h"
#include <fstream>
#include <sstream>
#include <filesystem>

StaticFileController::StaticFileController(const std::string& rootDir)
    : rootDir_(rootDir) {
    initMimeTypes();
}

HttpResponse StaticFileController::serveFile(const HttpRequest& req, const std::unordered_map<std::string, std::string>& params) {
    std::string path = rootDir_ + req.getPath();
    
    // 规范化路径
    std::filesystem::path fsPath = std::filesystem::absolute(path);
    path = fsPath.string();

    // 安全检查
    if (!isPathSafe(path)) {
        return createErrorResponse(HttpStatusCode::FORBIDDEN, "403 Forbidden");
    }

    // 如果是目录，尝试提供 index.html
    if (std::filesystem::is_directory(path)) {
        path += "/index.html";
    }

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        return createErrorResponse(HttpStatusCode::NOT_FOUND, "404 Not Found");
    }

    // 读取文件内容
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return createErrorResponse(HttpStatusCode::INTERNAL_SERVER_ERROR, "500 Internal Server Error");
    }

    std::ostringstream content;
    content << file.rdbuf();

    // 创建响应
    auto resp = HttpResponse::newHttpResponse();
    resp.setStatusCode(HttpStatusCode::OK);
    resp.setHeader("Content-Type", getMimeType(path));
    resp.setBody(content.str());

    // 设置 Cache-Control 头部 (可选)
    resp.setHeader("Cache-Control", "public, max-age=3600");

    return resp;
}

void StaticFileController::initMimeTypes() {
    mimeTypes_ = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"},
        {".xml", "application/xml"},
        {".mp3", "audio/mpeg"},
        {".mp4", "video/mp4"}
    };
}

std::string StaticFileController::getMimeType(const std::string& filename) const {
    auto ext = std::filesystem::path(filename).extension().string();
    auto it = mimeTypes_.find(ext);
    if (it != mimeTypes_.end()) {
        return it->second;
    }
    return "application/octet-stream";  // 默认 MIME 类型
}

bool StaticFileController::isPathSafe(const std::string& path) const {
    std::filesystem::path canonicalPath = std::filesystem::canonical(path);
    std::filesystem::path rootPath = std::filesystem::canonical(rootDir_);
    
    // 检查 canonicalPath 是否以 rootPath 开头
    auto rootPathStr = rootPath.string();
    auto canonicalPathStr = canonicalPath.string();
    
    if (canonicalPathStr.length() < rootPathStr.length()) {
        return false;  // 路径长度小于根目录，肯定不在根目录内
    }
    
    // 使用 string 的 compare 函数检查前缀
    return canonicalPathStr.compare(0, rootPathStr.length(), rootPathStr) == 0;
}

HttpResponse StaticFileController::createErrorResponse(HttpStatusCode code, const std::string& message) const {
    auto resp = HttpResponse::newHttpResponse();
    resp.setStatusCode(code);
    resp.setHeader("Content-Type", "text/plain");
    resp.setBody(message);
    return resp;
}
