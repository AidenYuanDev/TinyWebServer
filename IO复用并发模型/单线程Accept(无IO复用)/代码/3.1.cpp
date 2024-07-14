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
