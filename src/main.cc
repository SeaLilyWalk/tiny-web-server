#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <csignal>
#include <cstring>
#include <string>
#include <memory>

#include "../include/server/server.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    std::shared_ptr<Server> serv = std::make_shared<Server>(atoi(argv[1]));
    serv->run();

    return 0;
}