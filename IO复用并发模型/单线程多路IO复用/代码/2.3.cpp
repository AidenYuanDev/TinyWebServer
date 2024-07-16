#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <vector>
using namespace std;
#define BUFFER_SIZE 1024
#define PORT 8080
#define MAX_CLIENTS 30

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd < 0) {
        cerr << "socket failed" << endl;
        return 0;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof server_addr) < 0) {
        cerr << "bind failed" << endl;
        close(server_fd);
        return 0;
    }

    if (listen(server_fd, 3) < 0) {
        cerr << "listen failed" << endl;
        close(server_fd);
    }

    fd_set fds_read;
    vector<int> clients;
    while (1) {
        // 清空fd集合
        FD_ZERO(&fds_read);
        ;

        // 添加服务端socket到fd集合
        FD_SET(server_fd, &fds_read);
        int sd_max = server_fd + 1;

        // 添加客户端socket到fd集合
        for (int client_fd : clients) {
            FD_SET(client_fd, &fds_read);
            sd_max = max(sd_max, client_fd + 1);
        }

        // 监听 select 监听多个套接字
        int activity = select(sd_max, &fds_read, nullptr, nullptr, nullptr);
        if (activity < 0) {
            cerr << "select error" << endl;
            return 0;
        }

        // 是否是新连接
        if (FD_ISSET(server_fd, &fds_read)) {
            int client_fd = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK);
            if (client_fd < 0) {
                cerr << "accept4 failed" << endl;
                return 0;
            }
            clients.emplace_back(client_fd);
            cout << "New connection, socket fd is " << client_fd << endl;
        }

        // 处理客户端发来的消息
        for (auto it = clients.begin(); it != clients.end();) {
            int client_fd = *it;
            bool should_remove = false;

            if (FD_ISSET(client_fd, &fds_read)) {
                string buffer(BUFFER_SIZE, 0);
                int read_len = read(client_fd, buffer.data(), BUFFER_SIZE);
                if (read_len < 0) {
                    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                        cerr << "Read error: " << endl;
                        should_remove = true;
                    }
                } else if (read_len == 0) {
                    // 连接断开
                    cout << "Connection closed" << endl;
                    should_remove = true;
                } else {
                    cout << "Received:" << buffer.substr(0, read_len) << endl;
                    // 回显收到的消息
                    string message = "Echo:" + buffer.substr(0, read_len);
                    write(client_fd, message.data(), message.size());
                }
            }
            if (should_remove) {
                close(client_fd);
                it = clients.erase(it);
            } else ++it;
        }
    }
    close(server_fd);
    return 0;
}
