// router.cpp
#include "router.h"
#include <sstream>

Router::Router() = default;

void Router::addRoute(const std::string& path, HttpMethod method, RequestHandler handler) {
    routes_.emplace_back(std::make_unique<Route>(path, method, std::move(handler)));
    updateTrie(routes_.back().get());
}

std::pair<const Route*, std::unordered_map<std::string, std::string>> 
Router::matchRoute(const HttpRequest& request) const {
    const auto& path = request.getPath();
    const auto method = request.getMethod();
    
    auto node = &roots_[static_cast<int>(method)];
    std::vector<std::string> pathSegments = splitPath(path);
    
    for (const auto& segment : pathSegments) {
        if (node->children.count(segment) > 0) {
            node = node->children.at(segment).get();
        } else if (node->paramChild) {
            node = node->paramChild.get();
        } else {
            return {nullptr, {}};
        }
    }

    for (const auto& route : node->routes) {
        if (route->matches(pathSegments, method)) {
            return {route, route->extractParams(pathSegments)};
        }
    }

    return {nullptr, {}};
}

void Router::updateTrie(const Route* route) {
    auto& root = roots_[static_cast<int>(route->getMethod())];
    auto node = &root;
    std::vector<std::string> segments = splitPath(route->getPath());

    for (const auto& segment : segments) {
        if (segment[0] == ':') {
            if (!node->paramChild) {
                node->paramChild = std::make_unique<TrieNode>();
            }
            node = node->paramChild.get();
        } else {
            if (node->children.count(segment) == 0) {
                node->children[segment] = std::make_unique<TrieNode>();
            }
            node = node->children[segment].get();
        }
    }
    node->routes.push_back(route);
}

std::vector<std::string> Router::splitPath(const std::string& path) const {
    std::vector<std::string> segments;
    std::istringstream iss(path);
    std::string segment;
    while (std::getline(iss, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    return segments;
}
