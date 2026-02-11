#include "../../include/core/connection.h"

#include <iostream>

#include "../protocol/http.cc"
#include "../protocol/websocket.cc"


Connection::Connection(int fd, int epfd): 
    recv_buf_(BUF_SIZE), send_buf_(BUF_SIZE) 
{
    fd_ = fd;
    epfd_ = epfd;
    events_ = EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP;
    recved_len_ = 0;
    sended_len_ = 0;
    state_ = HTTP;
}


bool Connection::HTTPHandler(std::shared_ptr<Http::Frame> frame) {
    int part_len, http_len;
    char* http_end, *buf_p = (char*)recv_buf_.data();
    while (1) {
        part_len = read(fd_, buf_p+recved_len_, BUF_SIZE-recved_len_);
        if (part_len == 0)
            break;
        if (part_len < 0) {
            std::cout << "read error" << std::endl;
            return false;
        }
        recved_len_ += part_len;
        if (recved_len_ >= BUF_SIZE) {
            std::cout << "buf overflow" << std::endl;
            return false;
        }
        http_len = Http::parseFrame(frame, buf_p);
        if (http_len != 0) {
            if (!frame->valid)
                return false;
            if (frame->request_type == UPDATE_TO_WS)
                state_ = WEBSOCKET;
            std::strcpy(buf_p, buf_p+http_len);
            recved_len_ -= http_len;
            break;
        }
    }
    return true;
}


bool Connection::WebsocketHandler(
    std::shared_ptr<Websocket::Frame> frame
) {
    int part_len, ws_len;
    char *buf_p = (char*)recv_buf_.data();
    while (1) {
        part_len = read(fd_, buf_p+recved_len_, BUF_SIZE-recved_len_);
        if (part_len == 0)
            break;
        if (part_len < 0) {
            std::cout << "read error" << std::endl;
            return false;
        }
        recved_len_ += part_len;
        if (recved_len_ >= BUF_SIZE) {
            std::cout << "buf overflow" << std::endl;
            return false;
        }
        int ws_len = Websocket::parseFrame(frame, buf_p, recved_len_);
        if (ws_len != 0) {
            if (!frame->valid)
                return false;
            // std::cout << frame->data << std::endl;
            std::strcpy(buf_p, buf_p+ws_len);
            recved_len_ -= ws_len;
            break;
        }
    }
    return true;
}


bool Connection::SendData() {
    if (sended_len_ > BUF_SIZE) {
        std::cerr << "send error" << std::endl;
        return false;
    }

    int n, send_pos = 0;
    char *buf_p = (char*)send_buf_.data();
    while (send_pos < sended_len_) {
        n = write(fd_, buf_p+send_pos, sended_len_-send_pos);
        if (n > 0)
            send_pos += n;
        else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::strcpy(buf_p, buf_p+send_pos);
                sended_len_ -= send_pos;
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

    sended_len_ = 0;
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


bool Connection::SendData(char *message) {
    char *p = (char*)send_buf_.data()+sended_len_;
    std::strncpy(p, message, BUF_SIZE-sended_len_);
    sended_len_ += std::strlen(message);
    return SendData();
}