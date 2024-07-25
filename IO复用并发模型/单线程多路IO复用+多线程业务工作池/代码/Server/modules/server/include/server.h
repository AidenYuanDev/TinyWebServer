#pragma once
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <unordered_map>

#include "client_context.h"
#include "thread_pool.h"

const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;

class Server {
public:
    Server(int port);
    void run();

private:
    void handleNewConnection();
    void handleClientEvent(epoll_event &event);
    void handleRead(int client_fd);
    void handleWrite(int client_fd);
    void removeClient(int client_fd);
    void modifyEpollEvent(int fd, uint32_t events);

    int server_fd;
    int epoll_fd;
    ThreadPool pool;
    unordered_map<int, shared_ptr<ClientContext>> clients;
    mutex clients_mutex;
};
