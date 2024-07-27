#include "server.h"
#include <cstdint>

// Server implementation
Server::Server(int port) : pool(4) {
    server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd < 0) {
        throw runtime_error("Socket creation failed");
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        throw runtime_error("Bind failed");
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        throw runtime_error("Listen failed");
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        close(server_fd);
        throw runtime_error("epoll_create1 failed");
    }

    epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        close(server_fd);
        close(epoll_fd);
        throw runtime_error("epoll_ctl failed");
    }
}

void Server::run() {
    vector<epoll_event> events(MAX_EVENTS);

    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

        if (event_count < 0) {
            cerr << "epoll_wait failed: " << endl;
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

void Server::handleNewConnection() {
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept4(server_fd, (sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
        if (client_fd < 0) {
            if (errno == EAGAIN || EWOULDBLOCK) {
                cout << "No more new connections to accept" << endl;
                break;
            } else {
                cerr << "Accept failed: " << endl;
                break;
            }
        }

        epoll_event event;
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
        if (epoll_ctl_result < 0) {
            cerr << "epoll_ctl failed for client socket: " << endl;
            close(client_fd);
        } else {
            {
                lock_guard<mutex> lock(clients_mutex);
                clients[client_fd] = make_shared<ClientContext>();
            }
        }
    }
}

void Server::handleClientEvent(epoll_event &event) {
    int client_fd = event.data.fd;
    if (event.events & (EPOLLERR | EPOLLHUP)) {
        cout << "Error or hangup event for client: " << client_fd << endl;
        removeClient(client_fd);
    } else {
        if (event.events & EPOLLIN) handleRead(client_fd);
        if (event.events & EPOLLOUT) handleWrite(client_fd);   
    }
}

void Server::handleRead(int client_fd) {
    pool.enqueue([this, client_fd] {
        shared_ptr<ClientContext> client;
        {
            lock_guard<mutex> lock(clients_mutex);
            auto it = clients.find(client_fd);
            if (it == clients.end() || !it->second->isActive()) {
                cout << "Client " << client_fd << " not found or not active, skipping read handling" << endl;
                return;
            }
            client = it->second;
        }

        string buffer(BUFFER_SIZE, 0);
        while (true) {
            int read_len = read(client_fd, buffer.data(), buffer.size());
            if (read_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else {
                    cerr << "Read failed on socket " << client_fd << endl;
                    removeClient(client_fd);
                    break;
                }
            } else if (read_len == 0) {
                cout << "Client disconnected: " << client_fd << endl;
                removeClient(client_fd);
                break;
            } else {
                cout << "Received from client " << client_fd << ": " << buffer.substr(0, read_len) << endl;
                string message = "Echo: " + buffer.substr(0, read_len);
                client->pushMessage(message);
                client->setWriteReady(true);
                modifyEpollEvent(client_fd, EPOLLIN | EPOLLOUT);
            }
        }
    });
}

void Server::handleWrite(int client_fd) {
    pool.enqueue([this, client_fd] {
        shared_ptr<ClientContext> client;
        {
            lock_guard<mutex> lock(clients_mutex);
            auto it = clients.find(client_fd);
            if (it == clients.end() || !it->second->isActive()) {
                cout << "Client " << client_fd << " not found or not active, skipping write handling" << endl;
                return;
            }
            client = it->second;
        }

        if (!client->isWriteReady()) return;

        bool keep_writing = true;
        while (keep_writing && client->hasMessages()) {
            string message = client->popMessage();
            size_t total_sent = 0;
            while (total_sent < message.size()) {
                int write_len = write(client_fd, message.data() + total_sent, message.size() - total_sent);
                if (write_len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        client->pushMessage(message.substr(total_sent));
                        keep_writing = false;
                        break;
                    } else {
                        cerr << "Write error on socket " << client_fd << endl;
                        removeClient(client_fd);
                        return;
                    }
                } else total_sent += write_len;
            }
            if (total_sent == message.size()) 
                cout << "Sent to client " << client_fd << ": " << message << endl;
        }

        if (!client->hasMessages()) {
            client->setWriteReady(false);
            modifyEpollEvent(client_fd, EPOLLIN);
        }
    });
}

void Server::removeClient(int client_fd) {
    shared_ptr<ClientContext> client;
    {
        lock_guard<mutex> lock(clients_mutex);
        auto it = clients.find(client_fd);
        if (it != clients.end()) {
            client = it->second;
            clients.erase(it);
        }
    }

    if (client) {
        client->deactivate();
        int epoll_ctl_result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
        if (epoll_ctl_result < 0) 
            cerr << "Failed to remove client from epoll: " << endl;
        close(client_fd);
    }
}

void Server::modifyEpollEvent(int fd, uint32_t events) {
    epoll_event event;
        event.data.fd = fd;
        event.events = events | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0) 
            cerr << "Failed to modify epoll event for fd " << fd << endl;
}
