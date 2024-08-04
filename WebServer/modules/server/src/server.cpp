// modules/server/src/server.cpp

#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

Server::Server() {
    initializeServer();
    initializeMimeTypes();
}

void Server::initializeServer() {
    auto &config = ConfigManager::getInstance();

    int port = config.getPort();
    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd < 0) {
        LOG_FATAL("Socket creation failed");
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        LOG_FATAL("Bind failed");
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        LOG_FATAL("Listen failed");
        throw std::runtime_error("Listen failed");
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        close(server_fd);
        LOG_FATAL("epoll_create1 failed");
        throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        close(server_fd);
        close(epoll_fd);
        LOG_FATAL("epoll_ctl failed");
        throw std::runtime_error("epoll_ctl failed");
    }

    publicDirectory = config.getPublicDirectory();
    pool = std::make_unique<ThreadPool>();

    LOG_INFO("Server initialized on port %d with %d threads", port, config.getThreadPoolSize());
}

void Server::run() {
    LOG_INFO("Server starting...");
    std::vector<epoll_event> events(MAX_EVENTS);

    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            LOG_ERROR("epoll_wait failed: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < event_count; i++) {
            if (events[i].data.fd == server_fd) {
                handleNewConnection();
            } else {
                handleClientEvent(events[i]);
            }
        }
    }
}

void Server::registerHandler(HttpMethod method, const std::string &path, RequestHandler handler) {
    handlers[method][path] = std::move(handler);
    LOG_DEBUG("Registered handler for method %d, path %s", static_cast<int>(method), path.c_str());
}

void Server::handleNewConnection() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept4(server_fd, (sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                LOG_ERROR("Accept failed: %s", strerror(errno));
                break;
            }
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        LOG_INFO("New connection from %s:%d", client_ip, ntohs(client_addr.sin_port));

        epoll_event event;
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
        if (epoll_ctl_result < 0) {
            LOG_ERROR("epoll_ctl failed for client socket: %s", strerror(errno));
            close(client_fd);
        } else {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[client_fd] = std::make_shared<ClientContext>();
            LOG_DEBUG("Client %d added to epoll", client_fd);
        }
    }
}

void Server::handleClientEvent(epoll_event &event) {
    int client_fd = event.data.fd;
    if (event.events & (EPOLLERR | EPOLLHUP)) {
        if (event.events & EPOLLERR) {
            LOG_ERROR("Error event for client %d", client_fd);
        }
        if (event.events & EPOLLHUP) {
            LOG_INFO("Hangup event for client %d", client_fd);
        }
        removeClient(client_fd);
    } else {
        if (event.events & EPOLLIN) {
            LOG_DEBUG("Read event for client %d", client_fd);
            handleRead(client_fd);
        }
        if (event.events & EPOLLOUT) {
            LOG_DEBUG("Write event for client %d", client_fd);
            handleWrite(client_fd);
        }
    }
}

void Server::handleRead(int client_fd) {
    pool->enqueue([this, client_fd] {
        std::shared_ptr<ClientContext> client;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = clients.find(client_fd);
            if (it == clients.end() || !it->second->isActive()) {
                LOG_DEBUG("Client %d not found or not active, skipping read handling", client_fd);
                return;
            }
            client = it->second;
        }

        std::vector<char> buffer(BUFFER_SIZE);
        while (true) {
            int read_len = read(client_fd, buffer.data(), buffer.size());
            if (read_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else {
                    LOG_ERROR("Read failed on socket %d: %s", client_fd, strerror(errno));
                    removeClient(client_fd);
                    break;
                }
            } else if (read_len == 0) {
                LOG_INFO("Client disconnected: %d", client_fd);
                removeClient(client_fd);
                break;
            } else {
                auto request = client->parser.parse(std::string_view(buffer.data(), read_len));
                if (request) {
                    LOG_DEBUG("Received request from client %d: %s %s", client_fd, toString(request->method).data(), request->url.c_str());
                    HttpResponse response = generateResponse(*request);
                    client->pushResponse(response);
                    client->setWriteReady(true);
                    modifyEpollEvent(client_fd, EPOLLIN | EPOLLOUT);
                }
            }
        }
    });
}

void Server::handleWrite(int client_fd) {
    pool->enqueue([this, client_fd] {
        std::shared_ptr<ClientContext> client;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = clients.find(client_fd);
            if (it == clients.end() || !it->second->isActive()) {
                return;
            }
            client = it->second;
        }

        if (!client->isWriteReady() || !client->hasResponses()) return;

        HttpResponse response = client->popResponse();
        std::string headers = response.toString();

        // 发送头部
        if (send(client_fd, headers.c_str(), headers.length(), 0) < 0) {
            LOG_ERROR("Failed to send headers to client %d: %s", client_fd, strerror(errno));
            removeClient(client_fd);
            return;
        }

        // 发送主体
        size_t total_sent = 0;
        while (total_sent < response.body.size()) {
            ssize_t sent = send(client_fd, response.body.data() + total_sent, response.body.size() - total_sent, 0);
            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 资源暂时不可用，稍后重试
                    continue;
                } else {
                    LOG_ERROR("Send error to client %d: %s", client_fd, strerror(errno));
                    removeClient(client_fd);
                    return;
                }
            }
            total_sent += sent;
        }

        LOG_DEBUG("Sent response to client %d: %d bytes", client_fd, total_sent);

        if (!client->hasResponses()) {
            client->setWriteReady(false);
            modifyEpollEvent(client_fd, EPOLLIN);
        }
    });
}

void Server::removeClient(int client_fd) {
    std::shared_ptr<ClientContext> client;
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        auto it = clients.find(client_fd);
        if (it != clients.end()) {
            client = it->second;
            clients.erase(it);
        }
    }

    if (client) {
        client->deactivate();
        int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        if (epoll_ctl_result < 0) {
            LOG_ERROR("Failed to remove client %d from epoll: %s", client_fd, strerror(errno));
        }
        close(client_fd);
        LOG_INFO("Client %d removed", client_fd);
    }
}

void Server::modifyEpollEvent(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0) {
        LOG_ERROR("Failed to modify epoll event for fd %d: %s", fd, strerror(errno));
    }
}

HttpResponse Server::generateResponse(const HttpRequest &request) {
    switch (request.method) {
        case HttpMethod::GET:
            return handleGetRequest(request);
        case HttpMethod::HEAD:
            return handleHeadRequest(request);
        case HttpMethod::POST:
            return handlePostRequest(request);
        case HttpMethod::PUT:
            return handlePutRequest(request);
        case HttpMethod::DELETE:
            return handleDeleteRequest(request);
        case HttpMethod::OPTIONS:
            return handleOptionsRequest(request);
        default:
            return createMethodNotAllowedResponse();
    }
}

HttpResponse Server::serveStaticFile(const HttpRequest &request) {
    std::filesystem::path filePath = publicDirectory + request.url;

    if (std::filesystem::is_directory(filePath)) {
        filePath /= "index.html";
    }

    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
        std::ifstream file(filePath, std::ios::binary);
        if (file) {
            HttpResponse response;
            response.status_code = 200;
            response.status_message = "OK";

            // 读取文件内容
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            response.body.resize(size);
            file.read(response.body.data(), size);

            response.headers["Content-Type"] = getMimeType(filePath.string());
            response.headers["Content-Length"] = std::to_string(response.body.size());
            addCommonHeaders(response);
            LOG_DEBUG("Serving static file: %s", filePath.string().c_str());
            return response;
        }
    }
    // 文件不存在，返回 404
    LOG_WARN("File not found: %s", filePath.string().c_str());
    return HttpResponseFactory::createNotFoundResponse();
}

std::string Server::getMimeType(const std::string &filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = filename.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        auto it = mimeTypes.find(ext);
        if (it != mimeTypes.end()) {
            return it->second;
        }
    }
    return "application/octet-stream";  // 默认二进制流
}

HttpResponse Server::handleGetRequest(const HttpRequest &request) {
    // 检查是否有注册的处理程序
    auto methodHandlers = handlers.find(HttpMethod::GET);
    if (methodHandlers != handlers.end()) {
        auto handler = methodHandlers->second.find(request.url);
        if (handler != methodHandlers->second.end()) {
            LOG_DEBUG("Custom handler found for GET request: %s", request.url.c_str());
            return handler->second(request);
        }
    }

    // 如果没有注册的处理程序，则尝试提供静态文件
    LOG_DEBUG("Serving static file for GET request: %s", request.url.c_str());
    return serveStaticFile(request);
}

HttpResponse Server::handleHeadRequest(const HttpRequest &request) {
    HttpResponse response = handleGetRequest(request);
    response.body.clear();  // HEAD 请求不返回响应体
    LOG_DEBUG("Handled HEAD request for: %s", request.url.c_str());
    return response;
}

HttpResponse Server::handlePostRequest(const HttpRequest &request) {
    auto methodHandlers = handlers.find(HttpMethod::POST);
    if (methodHandlers != handlers.end()) {
        auto handler = methodHandlers->second.find(request.url);
        if (handler != methodHandlers->second.end()) {
            LOG_DEBUG("Custom handler found for POST request: %s", request.url.c_str());
            return handler->second(request);
        }
    }
    LOG_WARN("No handler found for POST request: %s", request.url.c_str());
    return createMethodNotAllowedResponse();
}

HttpResponse Server::handlePutRequest(const HttpRequest &request) {
    auto methodHandlers = handlers.find(HttpMethod::PUT);
    if (methodHandlers != handlers.end()) {
        auto handler = methodHandlers->second.find(request.url);
        if (handler != methodHandlers->second.end()) {
            LOG_DEBUG("Custom handler found for PUT request: %s", request.url.c_str());
            return handler->second(request);
        }
    }
    LOG_WARN("No handler found for PUT request: %s", request.url.c_str());
    return createMethodNotAllowedResponse();
}

HttpResponse Server::handleDeleteRequest(const HttpRequest &request) {
    auto methodHandlers = handlers.find(HttpMethod::DELETE);
    if (methodHandlers != handlers.end()) {
        auto handler = methodHandlers->second.find(request.url);
        if (handler != methodHandlers->second.end()) {
            LOG_DEBUG("Custom handler found for DELETE request: %s", request.url.c_str());
            return handler->second(request);
        }
    }
    LOG_WARN("No handler found for DELETE request: %s", request.url.c_str());
    return createMethodNotAllowedResponse();
}

HttpResponse Server::handleOptionsRequest(const HttpRequest & /* request */) {
    HttpResponse response;
    response.status_code = 200;
    response.status_message = "OK";
    response.headers["Allow"] = "GET, HEAD, POST, PUT, DELETE, OPTIONS";
    addCommonHeaders(response);
    LOG_DEBUG("Handled OPTIONS request");
    return response;
}

HttpResponse Server::createMethodNotAllowedResponse() {
    HttpResponse response;
    response.status_code = 405;
    response.status_message = "Method Not Allowed";
    response.headers["Allow"] = "GET, HEAD, POST, PUT, DELETE, OPTIONS";
    addCommonHeaders(response);
    LOG_WARN("Returned 405 Method Not Allowed response");
    return response;
}

void Server::addCommonHeaders(HttpResponse &response) {
    response.headers["Server"] = "TinyWebServer/1.0";
    response.headers["Date"] = getCurrentDate();
    response.headers["Connection"] = "close";
}

std::string Server::getCurrentDate() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%a, %d %b %Y %H:%M:%S GMT");
    return ss.str();
}

void Server::initializeMimeTypes() {
    mimeTypes = {{".html", "text/html"},  {".css", "text/css"},   {".js", "application/javascript"}, {".json", "application/json"}, {".png", "image/png"},       {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"}, {".gif", "image/gif"}, {".svg", "image/svg+xml"}, {".ico", "image/x-icon"},
                 {".webp", "image/webp"}, {".txt", "text/plain"}, {".pdf", "application/pdf"},       {".xml", "application/xml"},   {".zip", "application/zip"}, {".mp3", "audio/mpeg"}, {".mp4", "video/mp4"}};
    LOG_INFO("MIME types initialized");
}
