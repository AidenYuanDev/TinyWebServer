#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

const int PORT = 8080;

int main() {
    // 创建socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket 创建失败" << endl;
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (!inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)) {
        cerr << "地址非法/地址不支持" << endl;
        close(sock);  // 确保资源被释放
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "连接失败" << endl;
        close(sock);  // 确保资源被释放
        return -1;
    }

    // 发送消息
    string msg = "请求文件file.txt";
    if (send(sock, msg.c_str(), msg.size(), 0) < 0) {
        cerr << "发送消息失败" << endl;
        close(sock);  // 确保资源被释放
        return -1;
    }
    cout << "向服务端发送消息：" << msg << endl;

    // 接收消息
    string buffer(1024, 0);
    int recv_len = recv(sock, buffer.data(), buffer.size(), 0);
    if (recv_len < 0) {
        cerr << "接收消息失败" << endl;
        close(sock);  // 确保资源被释放
        return -1;
    }
    cout << "从服务器接收消息：" << buffer.substr(0, recv_len) << endl;

    // 关闭socket
    close(sock);
    return 0;
}

