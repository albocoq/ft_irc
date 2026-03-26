#include "Server.hpp"
#include "Message.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
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
        std::cerr << "Socket error\n";
        exit(1);
    }

    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(_serverFd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind error\n";
        exit(1);
    }

    if (listen(_serverFd, SOMAXCONN) < 0) {
        std::cerr << "Listen error\n";
        exit(1);
    }

    pollfd p;
    p.fd = _serverFd;
    p.events = POLLIN;
    _fds.push_back(p);

    std::cout << "Server running on port " << _port << std::endl;
}

void Server::run() {
    while (true) {
        if (poll(&_fds[0], _fds.size(), -1) < 0) {
            std::cerr << "poll error\n";
            return;
        }

        for (size_t i = 0; i < _fds.size(); ) {
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            if (fd == _serverFd) {
                if (revents & POLLIN)
                    acceptClient();
                i++;
                continue;
            }

            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                disconnectClient(i);
                continue;
            }

            if (revents & POLLIN) {
                handleClientRead(i);
                if (i >= _fds.size() || _fds[i].fd != fd)
                    continue;
            }

            if (revents & POLLOUT) {
                handleClientWrite(i);
                if (i >= _fds.size() || _fds[i].fd != fd)
                    continue;
            }

            Client* client = getClientByFd(fd);
            if (!client)
                continue;

            _fds[i].events = POLLIN;
            if (!client->getWriteBuffer().empty())
                _fds[i].events |= POLLOUT;

            if (client->isToBeDisconnected() && client->getWriteBuffer().empty()) {
                disconnectClient(i);
                continue;
            }

            i++;
        }
    }
}

void Server::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    int clientFd = accept(_serverFd, (sockaddr*)&clientAddr, &len);
    if (clientFd < 0)
        return;

    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    std::string ip = inet_ntoa(clientAddr.sin_addr);

    Client* client = new Client(clientFd, ip);
    _clients.push_back(client);

    pollfd p;
    p.fd = clientFd;
    p.events = POLLIN;
    _fds.push_back(p);

    std::cout << "New client: " << clientFd << std::endl;
}

void Server::handleClientRead(size_t i) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(_fds[i].fd, buffer, sizeof(buffer), 0);

    Client* client = getClientByFd(_fds[i].fd);
    if (!client) {
        disconnectClient(i);
        return;
    }

    if (bytes == 0) {
        disconnectClient(i);
        return;
    }

    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(i);
        return;
    }

    client->appendReadBuffer(std::string(buffer, bytes));

    std::string line;
    while (client->hasCompleteLine()) {
        line = client->extractLine();
        Message msg(line);
        _handler.execute(*client, msg, _clients);
    }
}

void Server::handleClientWrite(size_t i) {
    Client* client = getClientByFd(_fds[i].fd);
    if (!client) {
        disconnectClient(i);
        return;
    }

    std::string data = client->getWriteBuffer();

    if (data.empty()) {
        if (client->isToBeDisconnected())
            disconnectClient(i);
        return;
    }

    int sent = send(_fds[i].fd, data.c_str(), data.size(), 0);

    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(i);
        return;
    }

    if (static_cast<size_t>(sent) < data.size())
        client->setWriteBuffer(data.substr(sent));
    else
        client->setWriteBuffer("");

    if (client->isToBeDisconnected() && client->getWriteBuffer().empty())
        disconnectClient(i);
}

void Server::disconnectClient(size_t i) {
    int fd = _fds[i].fd;

    for (size_t j = 0; j < _clients.size(); j++) {
        if (_clients[j]->getFd() == fd) {
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            break;
        }
    }

    close(fd);
    _fds.erase(_fds.begin() + i);

    std::cout << "Disconnected: " << fd << std::endl;
}

Client* Server::getClientByFd(int fd) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == fd)
            return _clients[i];
    }
    return NULL;
}