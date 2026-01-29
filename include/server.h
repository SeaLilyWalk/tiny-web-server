#ifndef SERVER_H
#define SERVER_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>

#include "../include/connection.h"

#define EPOLL_SIZE 50


class Server{
private:
    int serv_sock_, epfd_;
    struct epoll_event *ep_events_;
    std::unordered_map<int, Connection*> connections_; // 通过套接字监听，再调用类Connection解决连接问题

    void error_handler(std::string msg);
    void set_non_blocking_mode(int fd);

public:
    Server(int port);
    ~Server();
    void Run();
};

#endif