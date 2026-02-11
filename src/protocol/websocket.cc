#include "../../include/protocol/websocket.h"

#include <iostream>
#include <cstring>


size_t Websocket::parseFrame(
    std::shared_ptr<Websocket::Frame> frame, char *buf, int recved_len
) {
    int fin = buf[0] & 0x80;
    int opcode = buf[0] & 0x0f;
    int masked = buf[1] & 0x80;
    int payload_len = buf[1] & 0x7f;
    int ws_len = payload_len+6;
    if (recved_len < ws_len)
        return 0;
    if (payload_len > 125 || opcode != 0x01) {
        frame->valid = false;
        return ws_len;
    }
    unsigned char *mask = (unsigned char*)buf+2;
    unsigned char *payload = mask+4;
    for (int i = 0; i < payload_len; ++i)
        frame->data.push_back(payload[i]^mask[i%4]);
    frame->valid = true;
    frame->length = payload_len;
    return ws_len;
}


void Websocket::buildFrame(
        const std::vector<uint8_t>& data, std::vector<uint8_t>& buf,
        int payload_len
) {
    int pos = 0;
    buf[pos++] = 0x81;
    if (payload_len < 126) {
        buf[pos++] = payload_len;
    } else if (payload_len <= 0xFFFF) {
        buf[pos++] = 126;
        buf[pos++] = (payload_len >> 8) & 0xFF;
        buf[pos++] = payload_len & 0xFF;
    } else {
        buf[pos++] = 127;
        for (int i = 7; i >= 0; i--)
            buf[pos++] = (payload_len >> (i * 8)) & 0xFF;
    }
    memcpy(buf.data()+pos, data.data(), payload_len);
}