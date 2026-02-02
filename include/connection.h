#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
#include <bitset>

#define BUF_SIZE 8192
#define HTTP 0
#define WEBSOCKET 1

class Connection{
private:
    int fd_;
    uint32_t events_; // 记录当前fd_对应epoll监听事件的状态
    int epfd_;
    int recv_len_;
    int send_len_;
    char *recv_buf_;
    char *send_buf_;
    bool state_;

    bool WebResponse(char *http_data);
    bool UpdateToWebsocket(char *http_data);

public:
    Connection(int fd, int epfd);
    ~Connection();
    inline bool get_state() {
        return state_;
    }
    bool HTTPHandler();         // HTTP帧通过特殊字符定界
    bool WebsocketHandler();    // Websocket帧通过长度定界
    bool SendData();
};

#endif