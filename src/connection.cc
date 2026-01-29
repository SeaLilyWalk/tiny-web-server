#include "../include/connection.h"

#include <iostream>


Connection::Connection(int fd) {
    fd_ = fd;
    buf_len_ = 0;
    buf_ = new char[BUF_SIZE+4]();
    state_ = HTTP;
}


Connection::~Connection() {
    delete [] buf_;
}


bool Connection::HTTPHandler() {
    int part_len, http_len;
    char* http_end;
    while (1) {
        part_len = read(fd_, buf_+buf_len_, BUF_SIZE-buf_len_);
        if (part_len == 0)
            break;
        if (part_len < 0)
            return false;
        http_end = std::strstr(buf_+buf_len_, "\r\n\r\n");
        if (http_end != nullptr) {
            http_len = (int)(http_end-buf_) + 4; 
            char *http_data = new char[BUF_SIZE]();
            std::strncpy(http_data, buf_, http_len);
            if (std::strstr(http_data, "HTTP/") == nullptr)     // 检查收到的是否为HTTP报文
                return false;
            bool request_successful;
            if (std::strstr(http_data, "Sec-WebSocket-Key") == nullptr)
                request_successful = SendWeb(http_data);
            // else 
            //     request_successful = UpdateToWebsocket(http_data);
            delete [] http_data;
            std::strncpy(buf_, buf_+http_len, BUF_SIZE-http_len);
            if (!request_successful)
                return false;
        } else {
            buf_len_ += part_len;
            if (buf_len_ >= BUF_SIZE)                           // 检查收到的是否为HTTP报文
                return false;
        }
    }
    return true;
}


bool Connection::SendWeb(char *http_data) {
    std::cout << http_data << std::endl;
    bool flag = false;
    if (std::strstr(http_data, "Accept: text/html") != nullptr)  {
        std::cout << "Accept: text/html" << std::endl;
        flag = true;
    }
    char req_line[64] = {'\0'};
    std::strcpy(req_line, std::strtok(http_data, "\r\n"));
    std::cout << req_line << std::endl;
    if (std::strcmp("GET", std::strtok(req_line, " ")) != 0)
        return false;
    char file_name[30] = "pages";
    std::cout << file_name << std::endl;
    std::strncpy(file_name+5, std::strtok(NULL, " "), 25);
    std::cout << file_name << std::endl;
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
    std::cout << "write head" << std::endl;
    if (flag) {
        memset(file_buf, 0, 128);
        while (fgets(file_buf, 128, send_web) != NULL) {
            write(fd_, file_buf, sizeof(file_buf));
            std::cout << file_buf;
            memset(file_buf, 0, 128);
        }
        std::cout << std::endl;
     }
    fclose(send_web);
    return true;
}