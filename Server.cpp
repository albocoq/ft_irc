#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverFd(-1), _handler(password) {}

Server::~Server() {
    if (_serverFd >= 0)
        close(_serverFd);
    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
}

void Server::initServer() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) {
        std::cerr << "Error creating socket\n";
        exit(1);
    }

    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: setsockopt failed\n";
        exit(1);
    }

    // Non-blocking
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl failed\n";
        exit(1);
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        exit(1);
    }

    if (listen(_serverFd, SOMAXCONN) < 0) {
        std::cerr << "Listen failed\n";
        exit(1);
    }

    // Añadir server_fd a poll
    pollfd p;
    p.fd = _serverFd;
    p.events = POLLIN;
    _fds.push_back(p);

    std::cout << "Server started on port " << _port << std::endl;
}