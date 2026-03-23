#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _handler(password) {}

Server::~Server() {
    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
    close(_serverFd);
}