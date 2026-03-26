#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <string>
#include <poll.h>
#include "Client.hpp"
#include "CommandHandler.hpp"

class Server {
private:
    int _port;
    std::string _password;
    int _serverFd;

    std::vector<pollfd> _fds;
    std::vector<Client*> _clients;

    CommandHandler _handler;

public:
    Server(int port, const std::string& password);
    ~Server();

    void initServer();
    void run();

private:
    void acceptClient();
    void handleClientRead(size_t i);
    void handleClientWrite(size_t i);
    void disconnectClient(size_t i);
    Client* getClientByFd(int fd);
};

#endif