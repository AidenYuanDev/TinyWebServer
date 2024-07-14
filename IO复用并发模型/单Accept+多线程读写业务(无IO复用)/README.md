[toc

# 一、目录
相较前一篇文章在服务端加上多线程就行。
[TinyWebServer](https://github.com/AidenYuanDev/TinyWebServer)

# 二、实现
![单Accept+多线程读写业务(无IO复用)](图片/单Accept+多线程读写业务(无IO复用).png)
- **优点：** 每个连接都会有一个线程
- **缺点：** 
	
	- 对于高并发场景场景，线程数量受到硬件的瓶颈，线程过多也会增加CPU切换成本，降低CPU利用率
	- 仅适合客户端数量不大，并且可控的场景使用

## 1、服务端代码
~~~c
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
using namespace std;
const int PORT = 8080;
    

void handle_client(int client_fd) {
    // I/O操作
    string buffer(1024, 0);
    int recv_len = recv(client_fd, buffer.data(), buffer.size(), 0);
    cout << "从客户端接收到的消息：" << buffer.substr(0, recv_len) << endl;

    string msg = "file.txt";
    send(client_fd, msg.data(), msg.size(), 0);
    cout << "向客户端发送消息：" << msg << endl;

    // close()
    close(client_fd);
}

int main(){
    int server_fd, client_fd;

    // socket()
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket 创建失败" << endl;
        return -1;
    }

    sockaddr_in address;
    int addrlen = sizeof address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT) ;

    // bind()
    if (bind(server_fd, (sockaddr*)&address, addrlen) < 0) {
        cerr << "Bind 失败" << endl;
        return -1;
    }

    // listen()
    if (listen(server_fd, 3) < 0){
        cerr << "Listen failed" << endl;
        return -1;
    } 
    cout << "服务器监听端口:" << PORT << endl;

    

    vector<thread> threads;

    while (1) {
    // accept()
        if ((client_fd = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            cerr << "Accept 失败" << endl;
            continue;
        }
        threads.emplace_back(handle_client, client_fd);
    }

    for (auto& t : threads) t.join();

    // close()
    close(server_fd);
    return 0;
}
~~~

## 2、客户端代码
这里不做变化沿用上一个的。
