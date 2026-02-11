#include "../../include/protocol/http.h"

#include <openssl/sha.h>
#include <openssl/evp.h>

#include <iostream>
#include <cstring>


size_t Http::parseFrame(std::shared_ptr<Http::Frame> frame, char* buf) {
    char* http_end = std::strstr(buf, "\r\n\r\n");
    if (http_end == nullptr)
        return 0;
    int http_len = (int)(http_end-buf) + 4; 
    char *http_data = new char[http_len+4]();
    std::strncpy(http_data, buf, http_len);
    if (std::strstr(http_data, "HTTP/") == nullptr) {
        frame->valid = false;
    } else if (std::strstr(http_data, "Sec-WebSocket-Key") == nullptr) {
        if (std::strstr(http_data, "Accept: text/html") != nullptr)
            frame->request_type = SEND_PAGE;
        else
            frame->request_type = NO_DATA;
        char req_line[64] = {'\0'};
        std::strcpy(req_line, std::strtok(http_data, "\r\n"));
        if (std::strcmp("GET", std::strtok(req_line, " ")) != 0) {
            frame->valid = false;
            delete [] http_data;
            return http_len;
        }
        char file_name[30] = "pages";
        std::strncpy(file_name+5, std::strtok(NULL, " "), 25);
        frame->data = file_name;
        frame->valid = true;
    } else {
        frame->request_type = UPDATE_TO_WS;
        char *ws_key = std::strtok(http_data, "\r\n");
        while (ws_key != nullptr) {
            ws_key = std::strtok(NULL, "\r\n");
            if (ws_key == nullptr) {
                frame->valid = false;
                delete [] http_data;
                return http_len;
            }
            if (std::strstr(ws_key, "Sec-WebSocket-Key") != nullptr)
                break;
        }
        std::strcpy(ws_key, ws_key+19); 
        frame->data = ws_key;
        frame->valid = true;
    }
    delete [] http_data;
    return http_len;
}


void Http::buildFrame(
    int code, 
    const std::vector<uint8_t>& data, std::vector<uint8_t>& buf
) {
    char* data_p = (char*)data.data();
    char* buf_p = (char*)buf.data();
    int pos = 0;
    if (code == 200) {
        pos = 84;
        char head200[] =    "HTTP/1.0 200 OK\r\n"
                            "Server:Linux Web Server\r\n"
                            "Content-length:8192\r\n"
                            "Content-type:html\r\n\r\n";
        std::strcpy(buf_p, head200);
    } else if (code == 404) {
        pos = 91;
        char head404[] =    "HTTP/1.0 404 Not Found\r\n"
                            "Server:Linux Web Server\r\n"
                            "Content-length:8192\r\n"
                            "Content-type:html\r\n\r\n";
        std::strcpy(buf_p, head404);
    } else {
        pos = 97;
        char head101[] =    "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Accept: ";
        std::strcpy(buf_p, head101);
    }
    // ws_accept额外加了"\r\n\r\n",这里就不用额外添加了
    std::strcpy(buf_p+pos, data_p);
}


void Http::get_ws_accept(char* ws_key, char *accept_key) {
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char buf[256];
    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    snprintf(buf, sizeof(buf), "%s%s", ws_key, GUID);
    SHA1((unsigned char *)buf, strlen(buf), sha1_result);
    EVP_EncodeBlock(
        (unsigned char *)accept_key,
        sha1_result,
        SHA_DIGEST_LENGTH
    );
    int len = std::strlen(accept_key);
    std::strcpy(accept_key+len, "\r\n\r\n");
}