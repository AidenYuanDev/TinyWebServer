#pragma once

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "message_queue.h"
#include "thread_pool.h"
#include "router.h"
#include "static_file_controller.h"

class IServer {
public:
    virtual ~IServer() = default;
    virtual void run() = 0;
};

class Server : public IServer {
public:
    Server(int port, std::string& publicDirectory, int threadPoolSize);
    void run() override;
    void registerHandler(HttpMethod method, const std::string &path, RequestHandler handler);

private:
    static constexpr std::size_t MAX_EVENTS = 2048;
    static constexpr std::size_t BUFFER_SIZE = 8192; // 8KB

    int server_fd;
    int epoll_fd;
    std::unique_ptr<ThreadPool> pool;
    std::unordered_map<int, std::unique_ptr<MessageQueue>> clients;
    std::mutex clients_mutex;
    Router router;
    std::unique_ptr<StaticFileController> staticFileController;
    std::string publicDirectory;

    void initializeServer(int port, std::string& publicDirectory, int threadPoolSize);
    void handleNewConnection();
    void handleClientEvent(epoll_event &event);
    void handleRead(int client_fd);
    void handleWrite(int client_fd);
    void removeClient(int client_fd);
    void modifyEpollEvent(int fd, uint32_t events);
    
    HttpResponse generateResponse(const HttpRequest &request);
    void addCommonHeaders(HttpResponse &response);
    std::string getCurrentDate() const;
};
