#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <iostream>
#include <csignal>
#include <cstring>
#include <string>

#include "../include/server.h"

Server *serv;

void signalHandler(int signum) {
    std::cout << "Recieved signal " << signum << std::endl;
    delete serv;
    exit(signum);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    serv = new Server(atoi(argv[1]));
    // std::signal(SIGINT, signalHandler);
    serv->Run();

    return 0;
}