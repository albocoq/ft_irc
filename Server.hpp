#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <vector>
#include <string>
#include <poll.h>
#include "Client.hpp"
#include "CommandHandler.hpp"

class Server {
    
    public:
        // Crea el servidor con puerto y password iniciales.
        Server(int port, const std::string& password);
        // Libera socket del servidor y clientes conectados.
        ~Server();

        // Inicializa socket, bind, listen y poll del servidor.
        void initServer();
        // Ejecuta el bucle principal de eventos con poll.
        void run();

    private:
        int _port;
        std::string _password;
        int _serverFd;
        int _nextClientId;
        std::vector<pollfd> _fds;
        std::vector<Client*> _clients;

        CommandHandler _handler;

        // Acepta una nueva conexion y la agrega a listas internas.
        void acceptClient();
        // Lee datos del cliente, parsea lineas y ejecuta comandos.
        void handleClientRead(size_t i);
        // Escribe buffer pendiente al cliente y gestiona desconexion.
        void handleClientWrite(size_t i);
        void flushPendingWrites();
        void disconnectClient(size_t i);
        // Busca y devuelve el cliente asociado a un file descriptor.
        Client* getClientByFd(int fd);
};

#endif