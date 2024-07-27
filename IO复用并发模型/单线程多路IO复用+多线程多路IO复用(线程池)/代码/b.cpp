
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

const int PORT = 8080;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;

class ThreadSafeQueue {
public:
    void push(int fd) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(fd);
        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        int fd = queue_.front();
        queue_.pop();
        return fd;
    }

private:
    std::queue<int> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

class Server {
public:
    Server(int port, int thread_count) 
        : port_(port), thread_count_(thread_count), should_stop_(false) {
        if (pipe(stop_pipe_) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    ~Server() {
        close(stop_pipe_[0]);
        close(stop_pipe_[1]);
    }

    void run() {
        create_listen_socket();
        create_thread_pool();
        main_loop();
    }

    void stop() {
        should_stop_ = true;
        write(stop_pipe_[1], "stop", 4); // Signal the worker threads to stop
        for (auto& thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    void create_listen_socket() {
        listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd_ == -1) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(listen_fd_, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(listen_fd_, SOMAXCONN) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << port_ << std::endl;
    }

    void create_thread_pool() {
        for (int i = 0; i < thread_count_; ++i) {
            threads_.emplace_back(&Server::worker_thread, this);
        }
    }

    void main_loop() {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            perror("epoll_create1 failed");
            exit(EXIT_FAILURE);
        }

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = listen_fd_;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd_, &ev) == -1) {
            perror("epoll_ctl: listen_fd_");
            exit(EXIT_FAILURE);
        }

        ev.events = EPOLLIN;
        ev.data.fd = stop_pipe_[0];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stop_pipe_[0], &ev) == -1) {
            perror("epoll_ctl: stop_pipe_");
            exit(EXIT_FAILURE);
        }

        struct epoll_event events[MAX_EVENTS];
        while (!should_stop_) {
            int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
            if (nfds == -1) {
                if (errno == EINTR) continue;
                perror("epoll_wait");
                break;
            }

            for (int n = 0; n < nfds; ++n) {
                if (events[n].data.fd == listen_fd_) {
                    handle_new_connection();
                } else if (events[n].data.fd == stop_pipe_[0]) {
                    char buf[4];
                    read(stop_pipe_[0], buf, 4); // Clear the stop signal
                    break; // Exit the loop
                }
            }
        }

        close(epoll_fd);
    }

    void handle_new_connection() {
        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int conn_fd = accept4(listen_fd_, (struct sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
            if (conn_fd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;  // No more connections to accept
                } else {
                    perror("accept");
                    break;
                }
            }

            std::cout << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;

            // Distribute the new connection to a worker thread
            task_queue_.push(conn_fd);
        }
    }

    void worker_thread() {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1) {
            perror("epoll_create1 failed");
            exit(EXIT_FAILURE);
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = stop_pipe_[0];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, stop_pipe_[0], &ev) == -1) {
            perror("epoll_ctl: stop_pipe_");
            exit(EXIT_FAILURE);
        }

        struct epoll_event events[MAX_EVENTS];
        while (!should_stop_) {
            int conn_fd = task_queue_.pop();

            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = conn_fd;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
                perror("epoll_ctl: conn_fd");
                close(conn_fd);
                continue;
            }

            while (!should_stop_) {
                int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
                if (nfds == -1) {
                    if (errno == EINTR) continue;
                    perror("epoll_wait");
                    break;
                }

                for (int n = 0; n < nfds; ++n) {
                    if (events[n].data.fd == conn_fd) {
                        if (events[n].events & EPOLLIN) {
                            handle_client_data(epoll_fd, conn_fd);
                        }
                        if (events[n].events & (EPOLLHUP | EPOLLERR)) {
                            std::cout << "Client disconnected (EPOLLHUP | EPOLLERR)" << std::endl;
                            close(conn_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, nullptr);
                            break;
                        }
                    } else if (events[n].data.fd == stop_pipe_[0]) {
                        char buf[4];
                        read(stop_pipe_[0], buf, 4); // Clear the stop signal
                        break; // Exit the loop
                    }
                }
            }

            close(conn_fd);
        }

        close(epoll_fd);
    }

    void handle_client_data(int epoll_fd, int fd) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        while (true) {
            bytes_read = read(fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                std::cout << "Received from client: " << std::string(buffer, bytes_read);
                // Echo back to the client
                ssize_t bytes_written = write(fd, buffer, bytes_read);
                if (bytes_written < 0) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("write");
                        close(fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    }
                    break;
                }
            } else if (bytes_read == 0) {
                std::cout << "Client disconnected (read returned 0)" << std::endl;
                close(fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more data to read
                    break;
                } else {
                    perror("read");
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    break;
                }
            }
        }
    }

    int port_;
    int thread_count_;
    int listen_fd_;
    int stop_pipe_[2];
    std::atomic<bool> should_stop_;
    std::vector<std::thread> threads_;
    ThreadSafeQueue task_queue_;
};

int main() {
    Server server(PORT, 4);  // 4 worker threads
    server.run();
    server.stop();
    return 0;
}
