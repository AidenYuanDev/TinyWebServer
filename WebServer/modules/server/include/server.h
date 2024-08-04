// modules/server/include/server.h

#pragma once

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "client_context.h"
#include "http_parser.h"
#include "thread_pool.h"
#include "config_manager.h"
#include "logger.h"

class IServer {
public:
    virtual ~IServer() = default;
    virtual void run() = 0;
};

class Server : public IServer {
public:
    using RequestHandler = std::function<HttpResponse(const HttpRequest &)>;

    Server();
    void run() override;
    void registerHandler(HttpMethod method, const std::string &path, RequestHandler handler);

private:
    static constexpr std::size_t MAX_EVENTS = 2048;
    static constexpr std::size_t BUFFER_SIZE = 8192; // 8KB

    int server_fd;
    int epoll_fd;
    std::unique_ptr<ThreadPool> pool;
    std::unordered_map<int, std::shared_ptr<ClientContext>> clients;
    std::mutex clients_mutex;
    std::unordered_map<HttpMethod, std::unordered_map<std::string, RequestHandler>> handlers;
    std::string publicDirectory;
    std::unordered_map<std::string, std::string> mimeTypes;

    void initializeServer();
    void handleNewConnection();
    void handleClientEvent(epoll_event &event);
    void handleRead(int client_fd);
    void handleWrite(int client_fd);
    void removeClient(int client_fd);
    void modifyEpollEvent(int fd, uint32_t events);
    
    HttpResponse generateResponse(const HttpRequest &request);
    HttpResponse handleGetRequest(const HttpRequest &request);
    HttpResponse handleHeadRequest(const HttpRequest &request);
    HttpResponse handlePostRequest(const HttpRequest &request);
    HttpResponse handlePutRequest(const HttpRequest &request);
    HttpResponse handleDeleteRequest(const HttpRequest &request);
    HttpResponse handleOptionsRequest(const HttpRequest &request);
    HttpResponse createMethodNotAllowedResponse();
    void addCommonHeaders(HttpResponse &response);
    std::string getCurrentDate() const;

    HttpResponse serveStaticFile(const HttpRequest &request);
    std::string getMimeType(const std::string& filename);

    void initializeMimeTypes();
};
