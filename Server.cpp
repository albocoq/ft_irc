#include "Server.hpp"
#include "Message.hpp"
#include <csignal>
#include "Client.hpp"


// Crea el servidor con el puerto y la clave que se usaran.
Server::Server(int port, const std::string& password)
    : _port(port), _password(password), _serverFd(-1), _nextClientId(1), _handler(password) {}

// Cierra el socket del servidor y libera los clientes guardados.
Server::~Server() {
    if (_serverFd >= 0)
        close(_serverFd);

    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
}

// Prepara el socket, lo enlaza al puerto y lo deja listo para recibir conexiones.
void Server::initServer() {
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) {
        std::cerr << "Socket error\n";
        exit(1);
    }

    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(_serverFd, F_GETFL, 0);// tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)
    if (flags >= 0)
        fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)

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

// Bucle principal: espera eventos y atiende lectura, escritura y desconexiones.
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

        flushPendingWrites();
    }
}

// Acepta un cliente nuevo, lo guarda y lo agrega al control de eventos.
void Server::acceptClient() {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    int clientFd = accept(_serverFd, (sockaddr*)&clientAddr, &len);
    if (clientFd < 0)
        return;

    int flags = fcntl(clientFd, F_GETFL, 0); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)
    if (flags >= 0)
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)

    std::string ip = inet_ntoa(clientAddr.sin_addr);
    int clientId = _nextClientId++;

    Client* client = new Client(clientId, clientFd, ip);
    _clients.push_back(client);
    client->appendWriteBuffer(":ircserv NOTICE * :You need to register with PASS, NICK and USER commands.");

    pollfd p;
    p.fd = clientFd;
    p.events = POLLIN;
    _fds.push_back(p);

    std::cout << "New client: " << clientId  << std::endl;
}

// Lee datos de un cliente, separa lineas completas y ejecuta sus comandos.
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

// Envia al cliente lo que tenga pendiente en su buffer de salida.
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

// Intenta vaciar mensajes pendientes de todos los clientes.
void Server::flushPendingWrites() {
    for (size_t i = 0; i < _fds.size(); ) {
        int fd = _fds[i].fd;

        if (fd == _serverFd) {
            i++;
            continue;
        }

        Client* client = getClientByFd(fd);
        if (!client || client->getWriteBuffer().empty()) {
            i++;
            continue;
        }

        handleClientWrite(i);
        if (i >= _fds.size() || _fds[i].fd != fd)
            continue;

        i++;
    }
}

// Desconecta un cliente, cierra su fd y lo elimina de las listas.
void Server::disconnectClient(size_t i) {
    int fd = _fds[i].fd;
    int disconnectedClientId = -1;

    for (size_t j = 0; j < _clients.size(); j++) {
        if (_clients[j]->getFd() == fd) {
            disconnectedClientId = _clients[j]->getId();
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            break;
        }
    }

    close(fd);
    _fds.erase(_fds.begin() + i);

    if (disconnectedClientId != -1)
        std::cout << "Disconnected: " << disconnectedClientId << std::endl;
    else
        std::cout << "Disconnected fd: " << fd << std::endl;
}

// Busca y devuelve el cliente que corresponde a un fd.
Client* Server::getClientByFd(int fd) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == fd)
            return _clients[i];
    }
    return NULL;
}