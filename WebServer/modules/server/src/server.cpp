// server.cpp

#include "server.h"
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream>
#include<filesystem>

Server::Server(int port) : pool(4) {
    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        throw std::runtime_error("Listen failed");
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        close(server_fd);
        throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        close(server_fd);
        close(epoll_fd);
        throw std::runtime_error("epoll_ctl failed");
    }

    setPublicDirectory("./public");  // 设置公共目录

    // 初始化MIME类型映射
    mimeTypes = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"}
    };
}

void Server::run() {
    std::vector<epoll_event> events(MAX_EVENTS);

    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
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
                std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                break;
            }
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "New connection from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

        epoll_event event;
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
        if (epoll_ctl_result < 0) {
            std::cerr << "epoll_ctl failed for client socket: " << strerror(errno) << std::endl;
            close(client_fd);
        } else {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[client_fd] = std::make_shared<ClientContext>();
            std::cout << "Client " << client_fd << " added to epoll" << std::endl;
        }
    }
}

void Server::handleClientEvent(epoll_event &event) {
    int client_fd = event.data.fd;
    if (event.events & (EPOLLERR | EPOLLHUP)) {
        if (event.events & EPOLLERR) {
            std::cerr << "Error event for client " << client_fd << std::endl;
        }
        if (event.events & EPOLLHUP) {
            std::cout << "Hangup event for client " << client_fd << std::endl;
        }
        removeClient(client_fd);
    } else {
        if (event.events & EPOLLIN) {
            std::cout << "Read event for client " << client_fd << std::endl;
            handleRead(client_fd);
        }
        if (event.events & EPOLLOUT) {
            std::cout << "Write event for client " << client_fd << std::endl;
            handleWrite(client_fd);
        }
    }
}

void Server::handleRead(int client_fd) {
    pool.enqueue([this, client_fd] {
        std::shared_ptr<ClientContext> client;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            auto it = clients.find(client_fd);
            if (it == clients.end() || !it->second->isActive()) {
                std::cout << "Client " << client_fd << " not found or not active, skipping read handling" << std::endl;
                return;
            }
            client = it->second;
        }

        std::string buffer(BUFFER_SIZE, 0);
        while (true) {
            int read_len = read(client_fd, buffer.data(), buffer.size());
            if (read_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else {
                    std::cerr << "Read failed on socket " << client_fd << std::endl;
                    removeClient(client_fd);
                    break;
                }
            } else if (read_len == 0) {
                std::cout << "Client disconnected: " << client_fd << std::endl;
                removeClient(client_fd);
                break;
            } else {
                auto request = client->parser.parse(std::string_view(buffer.data(), read_len));
                if (request) {
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
    pool.enqueue([this, client_fd] {
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
            std::cerr << "Failed to send headers" << std::endl;
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
                    std::cerr << "Send error" << std::endl;
                    removeClient(client_fd);
                    return;
                }
            }
            total_sent += sent;
        }

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
            std::cerr << "Failed to remove client from epoll: " << strerror(errno) << std::endl;
        }
        close(client_fd);
    }
}

void Server::modifyEpollEvent(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0) {
        std::cerr << "Failed to modify epoll event for fd " << fd << std::endl;
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
        // ... 其他方法 ...
        default:
            return createMethodNotAllowedResponse();
    }
}

void Server::setPublicDirectory(const std::string &path) {
    publicDirectory = path;
}

HttpResponse Server::serveStaticFile(const HttpRequest& request) {
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
            return response;
        }
    }

    // 文件不存在，返回 404
    HttpResponse response;
    response.status_code = 404;
    response.status_message = "Not Found";
    response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
    response.headers["Content-Type"] = "text/html";
    response.headers["Content-Length"] = std::to_string(response.body.size());
    return response;
}

std::string Server::getMimeType(const std::string& filename) {
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
    // 现有的静态文件服务逻辑
    return serveStaticFile(request);
}

HttpResponse Server::handleHeadRequest(const HttpRequest &request) {
    HttpResponse response = handleGetRequest(request);
    response.body.clear();  // HEAD 请求不返回响应体
    return response;
}

HttpResponse Server::handlePostRequest(const HttpRequest &request) {
    // 处理 POST 请求的逻辑
    // 例如，可以解析表单数据或 JSON 数据
    // ...
}

HttpResponse Server::handlePutRequest(const HttpRequest &request) {
    // 处理 PUT 请求的逻辑
    // 例如，更新资源
    // ...
}

HttpResponse Server::handleDeleteRequest(const HttpRequest &request) {
    // 处理 DELETE 请求的逻辑
    // 例如，删除资源
    // ...
}

HttpResponse Server::handleOptionsRequest(const HttpRequest &request) {
    HttpResponse response;
    response.status_code = 200;
    response.status_message = "OK";
    response.headers["Allow"] = "GET, HEAD, POST, PUT, DELETE, OPTIONS";
    return response;
}

HttpResponse Server::createMethodNotAllowedResponse() {
    HttpResponse response;
    response.status_code = 405;
    response.status_message = "Method Not Allowed";
    response.headers["Allow"] = "GET, HEAD, POST, PUT, DELETE, OPTIONS";
    return response;
}

void Server::addCommonHeaders(HttpResponse &response) {
    response.headers["Server"] = "YourServerName/1.0";
    response.headers["Date"] = getCurrentDate();  // 实现 getCurrentDate() 方法返回当前时间
    response.headers["Connection"] = "close";  // 或者 "keep-alive"，取决于您的实现
}
