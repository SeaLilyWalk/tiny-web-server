#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <cstdio>
#include <vector>
#include <cstring>
#include <string>
#include <memory>

#include "../protocol/http.h"
#include "../protocol/websocket.h"

#define BUF_SIZE 8192
#define HTTP 0
#define WEBSOCKET 1

class Connection{
private:
    int fd_;
    uint32_t events_; // 记录当前fd_对应epoll监听事件的状态
    int epfd_;
    int recved_len_;
    int sended_len_;
    std::vector<uint8_t> recv_buf_;
    std::vector<uint8_t> send_buf_;
    bool state_;

public:
    Connection(int fd, int epfd);
    inline bool get_state() {
        return state_;
    }
    inline int get_fd() {
        return fd_;
    }
    bool HTTPHandler(std::shared_ptr<Http::Frame> frame);
    bool WebsocketHandler(std::shared_ptr<Websocket::Frame> frame);
    bool SendData();
    bool SendData(char *message);
};

#endif