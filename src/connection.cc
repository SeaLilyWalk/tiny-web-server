#include "../include/connection.h"

#include <iostream>


Connection::Connection(int fd, int epfd) {
    fd_ = fd;
    epfd_ = epfd;
    events_ = EPOLLIN|EPOLLET;
    recv_len_ = 0;
    send_len_ = 0;
    recv_buf_ = new char[BUF_SIZE+4]();
    send_buf_ = new char[BUF_SIZE+4]();
    state_ = HTTP;
}


Connection::~Connection() {
    delete [] recv_buf_;
    delete [] send_buf_;
}


bool Connection::HTTPHandler() {
    int part_len, http_len;
    char* http_end;
    while (1) {
        part_len = read(fd_, recv_buf_+recv_len_, BUF_SIZE-recv_len_);
        if (part_len == 0)
            break;
        if (part_len < 0) {
            std::cout << "read error" << std::endl;
            return false;
        }
        http_end = std::strstr(recv_buf_+recv_len_, "\r\n\r\n");
        if (http_end != nullptr) {
            http_len = (int)(http_end-recv_buf_) + 4; 
            char *http_data = new char[BUF_SIZE]();
            std::strncpy(http_data, recv_buf_, http_len);
            if (std::strstr(http_data, "HTTP/") == nullptr)     // 检查收到的是否为HTTP报文
                return false;
            if (std::strstr(http_data, "Sec-WebSocket-Key") == nullptr) {
                WebResponse(http_data);
                delete [] http_data;
                return false;
            } else {
                bool request_successful;
                request_successful = UpdateToWebsocket(http_data);
                delete [] http_data;
                std::strncpy(recv_buf_, recv_buf_+http_len, BUF_SIZE-http_len);
                if (!request_successful)
                    return false;
                else
                    return true;
            }
        } else {
            recv_len_ += part_len;
            if (recv_len_ >= BUF_SIZE) {
                std::cout << "buf overflow" << std::endl;
                return false;
            }
        }
    }
    return true;
}


bool Connection::WebResponse(char *http_data) {
    bool flag = false;
    if (std::strstr(http_data, "Accept: text/html") != nullptr)  {
        flag = true;
    }
    char req_line[64] = {'\0'};
    std::strcpy(req_line, std::strtok(http_data, "\r\n"));
    if (std::strcmp("GET", std::strtok(req_line, " ")) != 0)
        return false;
    char file_name[30] = "pages";
    std::strncpy(file_name+5, std::strtok(NULL, " "), 25);
    FILE *send_web = fopen(file_name, "r");
    char file_buf[128];
    if (send_web == nullptr) {          // 404 Not Found
        char head404[] =   "HTTP/1.0 404 Not Found\r\n"
                            "Server:Linux Web Server\r\n"
                            "Content-length:8192\r\n"
                            "Content-type:html\r\n\r\n";
        write(fd_, head404, 92);
        if (flag) {
            FILE *send_error = fopen("pages/notfound.html", "r");
            if (send_error == nullptr)
                return false;
            while (fgets(file_buf, 128, send_error) != NULL)
                write(fd_, file_buf, sizeof(file_buf));
            fclose(send_error);
        }
        return false;
    }
    char head200[] =   "HTTP/1.0 200 OK\r\n"
                        "Server:Linux Web Server\r\n"
                        "Content-length:8192\r\n"
                        "Content-type:html\r\n\r\n";
    write(fd_, head200, 85);
    if (flag) {
        memset(file_buf, 0, 128);
        while (fgets(file_buf, 128, send_web) != NULL) {
            write(fd_, file_buf, std::strlen(file_buf));
            // std::cout << file_buf;
            memset(file_buf, 0, 128);
        }
     }
    fclose(send_web);
    return true;
}


bool Connection::UpdateToWebsocket(char *http_data) {
    char head101[140] = "HTTP/1.1 101 Switching Protocols\r\n"
                        "Upgrade: websocket\r\n"
                        "Connection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: ";
    char *ws_key = std::strtok(http_data, "\r\n");
    while (ws_key != nullptr) {
        ws_key = std::strtok(NULL, "\r\n");
        if (ws_key == nullptr)
            return false;
        if (std::strstr(ws_key, "Sec-WebSocket-Key") != nullptr)
            break;
    }
    std::strcpy(ws_key, ws_key+19); 

    // get Sec-WebSocket-Accept
    const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char buf[256];
    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    snprintf(buf, sizeof(buf), "%s%s", ws_key, GUID);
    SHA1((unsigned char *)buf, strlen(buf), sha1_result);
    char *accept_key = new char[64]();
    EVP_EncodeBlock(
        (unsigned char *)accept_key,
        sha1_result,
        SHA_DIGEST_LENGTH
    );

    snprintf(head101+97, 42, "%s%s", accept_key, "\r\n\r\n");
    std::strncpy(send_buf_+send_len_, head101, BUF_SIZE+4-send_len_);
    send_len_ += std::strlen(head101);

    delete [] accept_key;
    state_ = WEBSOCKET;
    if (!SendData())
        return false;
    return true;
}


bool Connection::WebsocketHandler() {
    int part_len;
    int fin, opcode, masked, payload_len;
    bool finded_head = false;
    unsigned char *p = (unsigned char*) recv_buf_;
    while (1) {
        part_len = read(fd_, recv_buf_+recv_len_, BUF_SIZE-recv_len_);
        if (part_len == 0)
            break;
        if (part_len < 0) {
            std::cout << "read error" << std::endl;
            return false;
        }
        recv_len_ += part_len;
        if (!finded_head && recv_len_ >= 2) {
            fin = p[0] & 0x80;
            opcode = p[0] & 0x0f;
            masked = p[1] & 0x80;
            payload_len = p[1] & 0x7f;
            finded_head = true;
            if (payload_len > 125) {
                std::cout << "payload too long" << std::endl;
                return false;
            } else if (opcode != 0x01) {
                std::cout << "error option" << std::endl;
                return false;
            }
        }
        if (recv_len_ >= BUF_SIZE) {
            std::cout << "buf overflow" << std::endl;
            return false;
        } else if (finded_head && recv_len_ >= payload_len+6) { // received the whole bag
            unsigned char *mask = p+2, *payload = p+6;
            for (int i = 0; i < payload_len; ++i)
                payload[i] ^= mask[i%4];
            char *msg = new char[payload_len+2]();
            std::strncpy(msg, (char*)payload, payload_len);
            msg[payload_len] = '\0';
            std::cout << "Websocket message: " << msg << std::endl;
            std::strcpy(recv_buf_, recv_buf_+payload_len+6);
            recv_len_ -= payload_len+6;
            // recv_buf_[recv_len_] = '\0';
            break;
        }
    }
    return true;
}


bool Connection::SendData() {
    if (send_len_ > BUF_SIZE+4) {
        std::cerr << "send error" << std::endl;
        return false;
    }

    int n, send_pos = 0;
    while (send_pos < send_len_) {
        n = write(fd_, send_buf_+send_pos, send_len_-send_pos);
        if (n > 0)
            send_pos += n;
        else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::strcpy(send_buf_, send_buf_+send_pos);
                send_len_ -= send_pos;
                if (events_&EPOLLOUT == 0) {
                    events_ |= EPOLLOUT;
                    struct epoll_event ev;
                    memset(&ev, 0, sizeof(ev));
                    ev.data.fd = fd_;
                    ev.events = events_;
                    epoll_ctl(epfd_, EPOLL_CTL_MOD, fd_, &ev);
                }
                return true;
            } else {
                std::cerr << "send error" << std::endl;
                return false;
            }
        }
    }

    send_len_ = 0;
    if (events_&EPOLLOUT) {
        events_ &= ~EPOLLOUT;
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = events_;
        ev.data.fd = fd_;
        epoll_ctl(epfd_, EPOLL_CTL_MOD, fd_, &ev);
    }
    return true;
}