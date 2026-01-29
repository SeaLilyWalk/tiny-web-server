#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>
#include <cstring>

#define BUF_SIZE 8192
#define HTTP 0
#define WEBSOCKET 1

class Connection{
private:
    int fd_;
    int buf_len_;
    char *buf_;
    bool state_;

    bool SendWeb(char *http_data);
    bool UpdateToWebsocket(char *http_data);

public:
    Connection(int fd);
    ~Connection();
    inline bool get_state() {
        return state_;
    }
    bool HTTPHandler();         // HTTP帧通过特殊字符定界
    bool WebsocketHandler();    // Websocket帧通过长度定界
};

#endif