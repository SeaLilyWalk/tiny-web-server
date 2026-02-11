#include "../../include/server/server.h"

#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_set>

#include "../../include/core/connection.h"
#include "../../include/protocol/http.h"

#define EPOLL_SIZE 50


Server::Server(int port): ep_events_(EPOLL_SIZE) {
    serv_sock_ = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(serv_sock_, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handler("bind() error");
    if (listen(serv_sock_, 5) == -1)
        error_handler("listen() error");

    epfd_ = epoll_create(EPOLL_SIZE);
    struct epoll_event event;
    event.events = EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP;
    event.data.fd = serv_sock_;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, serv_sock_, &event);
}


Server::~Server() {
    close(serv_sock_);
    for (auto &clnt : connections_) {
        close(clnt.first);
    }
    close(epfd_);
}


void Server::run() {
    int event_cnt;

    while(1) {
        event_cnt = epoll_wait(epfd_, ep_events_.data(), EPOLL_SIZE, -1);
        if (event_cnt == -1) {
            puts("epoll_wait() error");
            break;
        }
        for (int i = 0; i < event_cnt; ++i)
            doRequest(ep_events_[i]);
    } // while (1)
}


void Server::doRequest(epoll_event& event) {
    int curr_fd = event.data.fd;
    std::shared_ptr<Connection> curr_conn;
    if (curr_fd == serv_sock_) { // accept new link
        addConnecton();
        return;
        // 因为在建立链接之后浏览器才会发送报文, 所以先不用调用HTTPeHandler
    }
    if (event.events&(EPOLLERR|EPOLLHUP)) {
        removeConnection(curr_fd);
        return;
    }
    if (event.events&EPOLLIN) {
        curr_conn = connections_[curr_fd];
        if (curr_conn->get_state() == HTTP) {
            std::shared_ptr<Http::Frame> http_frame 
                = std::make_shared<Http::Frame>();
            if (!curr_conn->HTTPHandler(http_frame))
                removeConnection(curr_fd);
            else
                onHttpRequest(http_frame, curr_conn);
        } else {
            std::shared_ptr<Websocket::Frame> ws_frame
                = std::make_shared<Websocket::Frame>();
            if (!curr_conn->WebsocketHandler(ws_frame))
                removeConnection(curr_fd);
            else if (ws_frame->length != 0)
                broadcastWs(ws_frame, curr_fd);
        }
    }
    if (event.events&EPOLLOUT) {
        curr_conn = connections_[curr_fd];
        if (!curr_conn->SendData())
            removeConnection(curr_fd);
    }
}


void Server::addConnecton() {
    socklen_t addr_sz;
    struct sockaddr_in clnt_addr;
    struct epoll_event event;
    addr_sz = sizeof(clnt_addr);
    int clnt_sock = accept(
        serv_sock_, (struct sockaddr*)&clnt_addr, &addr_sz
    );
    int flag = fcntl(clnt_sock, F_GETFL, 0);
    fcntl(clnt_sock, F_SETFL, flag|O_NONBLOCK);
    event.events = EPOLLIN|EPOLLET|EPOLLERR|EPOLLHUP;
    event.data.fd = clnt_sock;
    epoll_ctl(epfd_, EPOLL_CTL_ADD, clnt_sock, &event);
    std::cout << "Connect client: ";
    std::cout << ntohl(clnt_addr.sin_addr.s_addr) << ' ';
    std::cout << ntohs(clnt_addr.sin_port) << std::endl;
    connections_[clnt_sock] = 
        std::make_shared<Connection>(clnt_sock, epfd_);
}


void Server::removeConnection(int fd) {
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    connections_.erase(fd);
    std::cout << "Close client: " << fd << std::endl;
}


void Server::onHttpRequest(
    std::shared_ptr<Http::Frame> frame, std::shared_ptr<Connection> conn
) {
    int code = 200;
    std::vector<uint8_t> frame_data(BUF_SIZE);
    std::vector<uint8_t> http_frame(BUF_SIZE);
    char file_buf[128] = {'\0'};
    size_t pos = 0;
    char *file_p = (char*)frame_data.data();
    if (frame->request_type == UPDATE_TO_WS) {
        code = 101;
        Http::get_ws_accept(frame->data.data(), (char*)frame_data.data());
    }
    else if (frame->request_type == SEND_PAGE){
        FILE *page_data = fopen(frame->data.data(), "r");
        if (page_data == nullptr) {
            code = 404;
            page_data = fopen("pages/notfound.html", "r");
        }
        while (fgets(file_buf, 128, page_data) != NULL) {
            std::strcpy(file_p+pos, file_buf);
            pos += std::strlen(file_buf);
        }
        fclose(page_data);
    } 
    else {
        FILE *page_data = fopen(frame->data.data(), "r");
        if (page_data == nullptr)
            code = 404;
        else 
            fclose(page_data);
    }
    Http::buildFrame(code, frame_data, http_frame);
    conn->SendData((char*)http_frame.data());
    if (frame->request_type==SEND_PAGE || frame->request_type==NO_DATA)
        removeConnection(conn->get_fd());
}


void Server::broadcastWs(
    std::shared_ptr<Websocket::Frame> frame, int usr_id
) {
    std::vector<uint8_t> frame_data(BUF_SIZE);
    std::vector<uint8_t> ws_frame(BUF_SIZE);
    sprintf(
        (char*)frame_data.data(), "[%d]: %s", 
        usr_id, frame->data.data()
    );
    int payload_len = std::strlen((char*)frame_data.data());
    Websocket::buildFrame(frame_data, ws_frame, payload_len);
    for (auto &usr : connections_)
        if (
            usr.first != serv_sock_ && 
            usr.second->get_state() == WEBSOCKET
        ) {
            usr.second->SendData((char*)ws_frame.data());
        }
}


void Server::error_handler(std::string msg) {
    std::cerr << msg << std::endl;
    exit(1);
}