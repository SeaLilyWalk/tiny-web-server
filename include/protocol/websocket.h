#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <arpa/inet.h>

#include <iostream>
#include <vector>
#include <memory>

namespace Websocket {
    struct Frame {
        bool valid;
        short length;
        std::string data;
    };

    size_t parseFrame(std::shared_ptr<Frame> frame, char* buf, int recved_len);
    void buildFrame(
        const std::vector<uint8_t>& data, std::vector<uint8_t>& buf, 
        int payload_len
    );
}

#endif