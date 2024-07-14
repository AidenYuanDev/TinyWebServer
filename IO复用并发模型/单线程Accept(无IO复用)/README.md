@[toc]

# 一、前言
单线程Accept(无IO复用)是网络最基础的模型，常供学习使用。

下面是我的GitHub仓库，记录笔者从0开始到完整的高性能服务器构建过程，如有帮助给个`star`吧！
[TinyWebServer](https://github.com/AidenYuanDev/TinyWebServer)

# 二、I/O复用中最基础的知识点

## 1、流
1. **定义：** 可以进行I/O操作的内核对象，包括文件、管道、套接字
2. **流的入口：** 文件描述符(fd)

## 2、I/O操作
对流的读写操作

## 3、阻塞等待
1. **定义：** 在阻塞I/O模式下，I/O操作会导致进程或线程阻塞，直到I/O操作完成。
2. **优点：** 不占用CPU时间。
3. **缺点：** 同一阻塞，如果多个I/O同时完成，只能处理一个流的阻塞监听。

## 4、非阻塞，忙轮询
1. **定义：** 忙轮询是一种等待条件满足的方式，其中进程或线程不断地检查条件是否满足，而不进行任何休眠或让出CPU。
2. **缺点：** 占用CPU，系统资源

## 5、多路I/O复用
1. **定义：** 既能阻塞等待不浪费资源，也能够同一时刻监听多个I/O请求状态。

> 这个就是用多路I/O复用的原因。

# 三、单线程Accept(无IO复用)
这个就是最基础的阻塞等待模型。

- **INADDR_ANY：** 泛指本机的意思，也就是表示本机的所有IP
- **htons：** 主机字节序转化到二进制形式
- **inet_pton：** 转换IPv4地址从文本到二进制形式
![单Accept(无IO复用)](图片/单Accept(无IO复用).png)


## 1、服务端
绑定本地`ip`，监听本地端口

~~~c
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;
const int PORT = 8080;
    
int main(){
    int server_fd, new_socket;

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

    // accept()
    if ((new_socket = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        cerr << "Accept 失败" << endl;
        return -1;
    }

    // I/O操作
    string buffer(1024, 0);
    int recv_len = recv(new_socket, buffer.data(), buffer.size(), 0);
    cout << "从客户端接收到的消息：" << buffer.substr(0, recv_len) << endl;

    string msg = "file.txt";
    send(new_socket, msg.data(), msg.size(), 0);
    cout << "向客户端发送消息：" << msg << endl;

    // close()
    close(new_socket);
    close(server_fd);
    return 0;
}
~~~

## 2、客户端
绑定服务端`ip`，监听服务端端口

~~~c
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;
const int PORT = 8080;

int main(){
    // socket()
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket 创建失败" << endl;
        return -1;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (!inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)) {
        cerr << "地址非法/ 地址不支持" << endl;
        return -1;
    }

    // 连接
    if (connect(sock, (sockaddr*)&serv_addr, sizeof serv_addr) < 0) {
        cerr << "连接失败" << endl;
        return -1;
    }

    // I/O操作
    string msg = "请求文件file.txt";
    send(sock, msg.data(), msg.size(), 0);
    cout << "向服务端发送消息：" << msg << endl;

    string buffer(1024, 0);
    int recv_len = recv(sock, buffer.data(), buffer.size(), 0);
    cout << "从服务器接收消息：" << buffer.substr(0, recv_len) << endl;

    // close()
    close(sock);
    return 0;
}
~~~