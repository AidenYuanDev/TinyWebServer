#include "server.h"
#include "http_parser.h"
#include <config_manager.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

Server::Server(int port, std::string& publicDirectory, int threadPoolSize) {
    initializeServer(port, publicDirectory, threadPoolSize);
}

void Server::initializeServer(int port, std::string& publicDirectory, int threadPoolSize) {
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

    auto &config = ConfigManager::getInstance();
    this->publicDirectory = publicDirectory;
    pool = std::make_unique<ThreadPool>(threadPoolSize);
    staticFileController = std::make_unique<StaticFileController>(publicDirectory);

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
    router.addRoute(path, method, std::move(handler));
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
            clients[client_fd] = std::make_unique<MessageQueue>();
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
        std::vector<char> buffer(BUFFER_SIZE);
        HttpParser parser;
        bool keep_alive = true;

        while (keep_alive) {
            ssize_t bytes_read = read(client_fd, buffer.data(), buffer.size());
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                } else {
                    LOG_ERROR("Read failed on socket %d: %s", client_fd, strerror(errno));
                    removeClient(client_fd);
                    return;
                }
            } else if (bytes_read == 0) {
                LOG_INFO("Client disconnected: %d", client_fd);
                removeClient(client_fd);
                return;
            }

           parser.parse(buffer.data(), bytes_read);
            while(parser.hasCompletedRequest()) {
                auto request = parser.getCompletedRequest();
                auto response = generateResponse(*request);
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    if (clients.find(client_fd) != clients.end()) {
                        clients[client_fd]->pushResponse(std::move(response));
                    }
                }
                // Check if we should keep the connection alive
                auto connection_header = request->getHeader("Connection");
                keep_alive = (connection_header == "keep-alive");
            }
            modifyEpollEvent(client_fd, EPOLLIN | EPOLLOUT);
            if (!keep_alive) {
                break;
            }
        }
    });
}

void Server::handleWrite(int client_fd) {
    pool->enqueue([this, client_fd] {
        std::unique_ptr<MessageQueue>& queue = clients[client_fd];
        if (!queue) {
            return;
        }

        while (queue->hasResponses()) {
            HttpResponse response = queue->popResponse();
            std::string response_str = response.toString();

            size_t total_sent = 0;
            while (total_sent < response_str.length()) {
                ssize_t sent = send(client_fd, response_str.c_str() + total_sent, 
                                    response_str.length() - total_sent, 0);
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
        }

        modifyEpollEvent(client_fd, EPOLLIN);
    });
}

void Server::removeClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(client_fd);

    int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    if (epoll_ctl_result < 0) {
        LOG_ERROR("Failed to remove client %d from epoll: %s", client_fd, strerror(errno));
    }
    close(client_fd);
    LOG_INFO("Client %d removed", client_fd);
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
    auto [route, params] = router.matchRoute(request);
    if (route) {
        return route->getHandler()(request, params);
    }
    // 如果没有匹配的路由，尝试提供静态文件
    return staticFileController->serveFile(request, {});
}

void Server::addCommonHeaders(HttpResponse &response) {
    response.setHeader("Server", "TinyWebServer/1.0");
    response.setHeader("Date", getCurrentDate());
    response.setHeader("Connection", "close");
}

std::string Server::getCurrentDate() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::gmtime(&in_time_t), "%a, %d %b %Y %H:%M:%S GMT");
    return ss.str();
}
