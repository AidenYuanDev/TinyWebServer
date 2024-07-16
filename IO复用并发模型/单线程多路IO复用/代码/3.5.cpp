#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
using namespace std;
const int PORT = 8080;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;

struct Client_Context {
    queue<string> send_queue;
    bool write_ready = false;
};

void handle_error(int client_fd, int epoll_fd, unordered_map<int, Client_Context> &clients) {
    cerr << "Error on socket" << endl;
    close(client_fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    clients.erase(client_fd);
}

void handle_read(int client_fd, int epoll_fd, unordered_map<int, Client_Context> &clients) {
    string buffer(BUFFER_SIZE, 0);
    while (true) {
        int read_len = read(client_fd, buffer.data(), buffer.size());
        if (read_len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else {
                cerr << "read failed" << endl;
                handle_error(client_fd, epoll_fd, clients);
                break;
            }
        } else if (read_len == 0) {
            // 客户端断开连接
            handle_error(client_fd, epoll_fd, clients);
            break;
        } else {
            cout << "Received:" << buffer.substr(0, read_len) << endl;

            // 回显收到的消息
            string message = "Echo:" + buffer.substr(0, read_len);
            clients[client_fd].send_queue.push(message);
            clients[client_fd].write_ready = true;
        }
    }
}

void handle_write(int client_fd, Client_Context &client) {
    // 只有在写就绪时才执行写操作
    if (!client.write_ready) return;
    while (!client.send_queue.empty()) {
        string &message = client.send_queue.front();
        int write_len = write(client_fd, message.data(), message.size());
        if (write_len < 0) {
            // 写缓冲区已满，等待下一次 EPOLLOUT事件
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            else {
                cerr << "Write error" << endl;
                return;
            }
        } else client.send_queue.pop();
    }

    // 所有数据写入完成，重置写就绪标志
    client.write_ready = false;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd < 0) {
        cerr << "Socket creation failed" << endl;
        return 0;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed" << endl;
        close(server_fd);
    }

    if (listen(server_fd, 5) < 0) {
        cerr << "Listen failed" << endl;
        close(server_fd);
        return 0;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        cerr << "epoll_create1 failed" << endl;
        close(server_fd);
        return 0;
    }

    epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        cerr << "epoll_ctl failed" << endl;
        close(server_fd);
        close(epoll_fd);
        return 0;
    }

    vector<epoll_event> events(MAX_EVENTS);
    unordered_map<int, Client_Context> clients;

    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (event_count < 0) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < event_count; i++) {
            // 处理新连接
            if (events[i].data.fd == server_fd) {
                while (true) {
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof client_addr;
                    int client_fd = accept4(server_fd, (sockaddr *)&client_addr, &client_len, SOCK_NONBLOCK);
                    if (client_fd < 0) {
                        // 所用连接处理完毕
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        else {
                            cerr << "Accept failed" << endl;
                            break;
                        }
                    }
                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                        cerr << "epoll_ctl failed" << endl;
                        close(client_fd);
                    } else clients[client_fd] = Client_Context();
                }
            } else {
                int client_fd = events[i].data.fd;
                if (events[i].events & (EPOLLERR | EPOLLHUP))
                    handle_error(client_fd, epoll_fd, clients);
                else {
                    if (events[i].events & EPOLLIN) handle_read(client_fd, epoll_fd, clients);
                    if (events[i].events & EPOLLOUT) handle_write(client_fd, clients[client_fd]);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}
