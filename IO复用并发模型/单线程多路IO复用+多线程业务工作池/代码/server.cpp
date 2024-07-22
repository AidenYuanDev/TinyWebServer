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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>

using namespace std;

const int PORT = 8080;
const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;

class ThreadPool {
public:
    explicit ThreadPool(size_t threads);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type>;
    ~ThreadPool();

private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop;
};

class ClientContext {
public:
    void pushMessage(const string& msg);
    bool hasMessages() const;
    string popMessage();
    void setWriteReady(bool ready);
    bool isWriteReady() const;

private:
    queue<string> send_queue;
    bool write_ready = false;
    mutable mutex mtx;
};

class Server {
public:
    Server(int port);
    void run();

private:
    void initializeSocket();
    void initializeEpoll();
    void handleNewConnection();
    void handleClientEvent(epoll_event& event);
    void handleRead(int client_fd);
    void handleWrite(int client_fd);
    void removeClient(int client_fd);

    int server_fd;
    int epoll_fd;
    ThreadPool pool;
    unordered_map<int, unique_ptr<ClientContext>> clients;
};

// ThreadPool implementation
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for(size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            while(true) {
                function<void()> task;
                {
                    unique_lock<mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if(this->stop && this->tasks.empty()) return;
                    task = move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
    using return_type = typename result_of<F(Args...)>::type;
    auto task = make_shared<packaged_task<return_type()>>(
        bind(forward<F>(f), forward<Args>(args)...)
    );
    future<return_type> res = task->get_future();
    {
        unique_lock<mutex> lock(queue_mutex);
        if(stop) throw runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

ThreadPool::~ThreadPool() {
    {
        unique_lock<mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(thread &worker: workers) worker.join();
}

// ClientContext implementation
void ClientContext::pushMessage(const string& msg) {
    lock_guard<mutex> lock(mtx);
    send_queue.push(msg);
}

bool ClientContext::hasMessages() const {
    lock_guard<mutex> lock(mtx);
    return !send_queue.empty();
}

string ClientContext::popMessage() {
    lock_guard<mutex> lock(mtx);
    string msg = send_queue.front();
    send_queue.pop();
    return msg;
}

void ClientContext::setWriteReady(bool ready) {
    lock_guard<mutex> lock(mtx);
    write_ready = ready;
}

bool ClientContext::isWriteReady() const {
    lock_guard<mutex> lock(mtx);
    return write_ready;
}

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
    cout << "Server listening on port " << PORT << endl;

    while (true) {
        int event_count = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);
        if (event_count < 0) {
            perror("epoll_wait failed");
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else {
                cerr << "Accept failed" << endl;
                break;
            }
        }
        epoll_event event;
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
            cerr << "epoll_ctl failed for client socket" << endl;
            close(client_fd);
        } else {
            clients[client_fd] = make_unique<ClientContext>();
            cout << "New client connected: " << client_fd << endl;
        }
    }
}

void Server::handleClientEvent(epoll_event& event) {
    int client_fd = event.data.fd;
    if (event.events & (EPOLLERR | EPOLLHUP)) {
        removeClient(client_fd);
    } else {
        if (event.events & EPOLLIN) handleRead(client_fd);
        if (event.events & EPOLLOUT) handleWrite(client_fd);
    }
}

void Server::handleRead(int client_fd) {
    pool.enqueue([this, client_fd] {
        string buffer(BUFFER_SIZE, 0);
        while (true) {
            int read_len = read(client_fd, buffer.data(), buffer.size());
            if (read_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
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
                clients[client_fd]->pushMessage(message);
                clients[client_fd]->setWriteReady(true);
            }
        }
    });
}

void Server::handleWrite(int client_fd) {
    pool.enqueue([this, client_fd] {
        auto& client = clients[client_fd];
        if (!client->isWriteReady()) return;
        while (client->hasMessages()) {
            string message = client->popMessage();
            int write_len = write(client_fd, message.data(), message.size());
            if (write_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) return;
                else {
                    cerr << "Write error on socket " << client_fd << endl;
                    return;
                }
            } else {
                cout << "Sent to client " << client_fd << ": " << message << endl;
            }
        }
        client->setWriteReady(false);
    });
}

void Server::removeClient(int client_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    clients.erase(client_fd);
}

int main() {
    try {
        Server server(PORT);
        server.run();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
