#ifndef SERVER_H
#define SERVER_H

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "../core/connection.h"
#include "../protocol/http.h"

#define EPOLL_SIZE 50


class Server{
private:
    int serv_sock_, epfd_;
    std::vector<epoll_event> ep_events_;
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;

    void addConnecton();
    void removeConnection(int fd);
    void onHttpRequest(
        std::shared_ptr<Http::Frame> frame, 
        std::shared_ptr<Connection> conn
    );
    void broadcastWs(std::shared_ptr<Websocket::Frame> frame, int usr_id);

    void error_handler(std::string msg);

public:
    Server(int port);
    ~Server();
    void run();
    void doRequest(epoll_event& event);
};

#endif