#ifndef HTTP_H
#define HTTP_H

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#define NO_DATA 0
#define SEND_PAGE 1
#define UPDATE_TO_WS 2

namespace Http {
    struct Frame {
        bool valid;
        short request_type;
        std::string data;
    };

    size_t parseFrame(std::shared_ptr<Frame> frame, char* buf);
    void buildFrame(
        int code, 
        const std::vector<uint8_t>& data, std::vector<uint8_t>& buf
    );
    void get_ws_accept(char *ws_key, char *accept_key);
}

#endif