// router.h
#pragma once

#include "http_types.h"
#include "route.h"
#include "http_request.h"
#include <memory>
#include <vector>
#include <array>

class Router {
public:
    Router();
    ~Router() = default;

    // 禁用拷贝构造和赋值操作符
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    // 允许移动构造和赋值
    Router(Router&&) noexcept = default;
    Router& operator=(Router&&) noexcept = default;

    void addRoute(const std::string& path, HttpMethod method, RequestHandler handler);
    std::pair<const Route*, std::unordered_map<std::string, std::string>> 
    matchRoute(const HttpRequest& request) const;

private:
    struct TrieNode {
        std::unordered_map<std::string, std::unique_ptr<TrieNode>> children;
        std::unique_ptr<TrieNode> paramChild;
        std::vector<const Route*> routes;
    };

    std::array<TrieNode, static_cast<size_t>(HttpMethod::CONNECT) + 1> roots_;
    std::vector<std::unique_ptr<Route>> routes_;

    void updateTrie(const Route* route);
    std::vector<std::string> splitPath(const std::string& path) const;
};
