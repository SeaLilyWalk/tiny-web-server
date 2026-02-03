#include "../include/server.h"

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_set>

#include "../include/connection.h"

#define EPOLL_SIZE 50


Server::Server(int port) {
    serv_sock_ = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(serv_sock_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handler("bind() error");
    if (listen(serv_sock_, 5) == -1)
        error_handler("listen() error");

    epfd_ = epoll_create(EPOLL_SIZE);
    ep_events_ = new epoll_event[EPOLL_SIZE]();
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serv_sock_;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, serv_sock_, &event);
}


Server::~Server() {
    delete [] ep_events_;
    close(serv_sock_);
    for (auto &clnt : connections_) {
        close(clnt.first);
        delete clnt.second;
    }
    close(epfd_);
}


void Server::Run() {
    int event_cnt, clnt_sock;
    socklen_t addr_sz;
    struct sockaddr_in clnt_addr;
    struct epoll_event event;
    int curr_fd;
    Connection *curr_connection;

    while(1) {
        event_cnt = epoll_wait(epfd_, ep_events_, EPOLL_SIZE, -1);
        if (event_cnt == -1) {
            puts("epoll_wait() error");
            break;
        }
        for (int i = 0; i < event_cnt; ++i) {
            curr_fd = ep_events_[i].data.fd;
            if (curr_fd == serv_sock_) { // accept new link
                addr_sz = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock_, (struct sockaddr*)&clnt_addr, &addr_sz);
                set_non_blocking_mode(clnt_sock);
                event.events = EPOLLIN|EPOLLET;
                event.data.fd = clnt_sock;
                epoll_ctl(epfd_, EPOLL_CTL_ADD, clnt_sock, &event);
                std::cout << "Connect client: ";
                std::cout << ntohl(clnt_addr.sin_addr.s_addr) << ' ' << ntohs(clnt_addr.sin_port) << std::endl;
                connections_[clnt_sock] = new Connection(clnt_sock, epfd_);
                // 因为在建立链接之后浏览器才会发送报文, 所以先不用调用HTTPeHandler
            } else if (ep_events_[i].events&EPOLLOUT) {
                curr_connection = connections_[curr_fd];
                if (!curr_connection->SendData()) {
                    epoll_ctl(epfd_, EPOLL_CTL_DEL, curr_fd, NULL);
                    delete curr_connection;
                    close(curr_fd);
                    connections_.erase(curr_fd);
                    std::cout << "Close client: " << curr_fd << std::endl;
                }
            } else {
                curr_connection = connections_[curr_fd];
                if (curr_connection->get_state() == HTTP) {
                    if (!curr_connection->HTTPHandler()) {
                        epoll_ctl(epfd_, EPOLL_CTL_DEL, curr_fd, NULL);
                        delete curr_connection;
                        close(curr_fd);
                        connections_.erase(curr_fd);
                        std::cout << "Close client: " << curr_fd << std::endl;
                    } else {
                        if (curr_connection->get_state() == WEBSOCKET)
                            std::cout << "Update to websocket" << std::endl;
                    }
                } else {
                    unsigned char *ws_frame = nullptr;
                    if (!curr_connection->WebsocketHandler(ws_frame)) {
                        epoll_ctl(epfd_, EPOLL_CTL_DEL, curr_fd, NULL);
                        delete curr_connection;
                        close(curr_fd);
                        connections_.erase(curr_fd);
                        std::cout << "Close client: " << curr_fd << std::endl;
                    } else if (ws_frame != nullptr){
                        for (auto &c : connections_) 
                            if (
                                c.first != serv_sock_ && 
                                // c.first != curr_fd &&
                                !c.second->SendData((char*)ws_frame)
                            ) {
                                epoll_ctl(epfd_, EPOLL_CTL_DEL, c.first, NULL);
                                delete c.second;
                                close(c.first);
                                connections_.erase(c.first);
                                std::cout << "Close client: " << c.first << std::endl;
                            }
                    }
                    if (ws_frame != nullptr)
                        delete [] ws_frame;
                } // if (curr_connection->get_state() == HTTP) ... else ...
            } // different EPOLL events
        } // for (int i = 0; i < event_cnt; ++i)
    } // while (1)
}


void Server::error_handler(std::string msg) {
    std::cerr << msg << std::endl;
    exit(1);
}


void Server::set_non_blocking_mode(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}