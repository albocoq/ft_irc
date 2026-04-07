#include "Server.hpp"
#include "Message.hpp"
#include "Client.hpp"


// Inicializa atributos base del servidor y el manejador de comandos.
Server::Server(int port, const std::string& password)
    // Guarda puerto, password y estado inicial del socket; crea CommandHandler.
    : _port(port), _password(password), _serverFd(-1), _handler(password) {}

// Cierra recursos abiertos del servidor y libera clientes en memoria.
Server::~Server() {
    // Si el socket del servidor esta abierto, lo cierra.
    if (_serverFd >= 0)
        close(_serverFd);

    // Libera cada cliente reservado dinamicamente.
    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
}

// Configura socket no bloqueante, bind/listen y registra fd en poll.
void Server::initServer() {
    // Crea socket TCP IPv4 para aceptar conexiones entrantes.
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    // Si falla la creacion del socket, informa y termina el proceso.
    if (_serverFd < 0) {
        std::cerr << "Socket error\n";
        exit(1);
    }

    // Activa SO_REUSEADDR para reutilizar el puerto al reiniciar.
    int opt = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(_serverFd, F_GETFL, 0);// tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)
    if (flags >= 0)
        fcntl(_serverFd, F_SETFL, flags | O_NONBLOCK); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)

    // Declara y limpia la estructura de direccion del servidor.
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));

    // Define familia IPv4, puerto de escucha y todas las interfaces.
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Asocia el socket a la direccion y puerto configurados.
    if (bind(_serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind error\n";
        exit(1);
    }

    // Pone el socket en modo escucha para conexiones nuevas.
    if (listen(_serverFd, SOMAXCONN) < 0) {
        std::cerr << "Listen error\n";
        exit(1);
    }

    // Registra el socket del servidor en poll para eventos de lectura.
    pollfd p;
    p.fd = _serverFd;
    p.events = POLLIN;
    _fds.push_back(p);

    // Muestra por consola que el servidor quedo operativo.
    std::cout << "Server running on port " << _port << std::endl;
}

// Atiende eventos de red y coordina lectura, escritura y desconexiones.
void Server::run() {
    // Bucle principal del servidor: procesa eventos indefinidamente.
    while (true) {
        // Espera hasta que algun fd tenga eventos listos.
        if (poll(&_fds[0], _fds.size(), -1) < 0) {
            std::cerr << "poll error\n";
            return;
        }

        // Recorre todos los descriptores vigilados por poll.
        for (size_t i = 0; i < _fds.size(); ) {
            // Guarda fd y eventos para evitar releer el vector varias veces.
            int fd = _fds[i].fd;
            short revents = _fds[i].revents;

            // Si es el fd del servidor, solo acepta nuevos clientes.
            if (fd == _serverFd) {
                if (revents & POLLIN)
                    acceptClient();
                i++;
                continue;
            }

            // Si el socket del cliente falla o cuelga, lo desconecta.
            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                disconnectClient(i);
                continue;
            }

            // Si hay datos entrantes, intenta leer y procesar mensajes.
            if (revents & POLLIN) {
                handleClientRead(i);
                // Si el cliente fue borrado, evita usar indice invalido.
                if (i >= _fds.size() || _fds[i].fd != fd)
                    continue;
            }

            // Si se puede escribir, vacia parte del buffer de salida.
            if (revents & POLLOUT) {
                handleClientWrite(i);
                // Si el cliente fue borrado, evita usar indice invalido.
                if (i >= _fds.size() || _fds[i].fd != fd)
                    continue;
            }

            // Obtiene el cliente asociado al fd actual.
            Client* client = getClientByFd(fd);
            if (!client)
                continue;

            // Por defecto siempre escucha lectura del cliente.
            _fds[i].events = POLLIN;
            // Si hay datos pendientes, tambien habilita escritura.
            if (!client->getWriteBuffer().empty())
                _fds[i].events |= POLLOUT;

            // Desconecta cuando se marco cierre y ya no queda nada por enviar.
            if (client->isToBeDisconnected() && client->getWriteBuffer().empty()) {
                disconnectClient(i);
                continue;
            }

            // Avanza al siguiente fd cuando este no fue eliminado.
            i++;
        }

        flushPendingWrites();
    }
}

// Acepta un cliente, lo pone en no bloqueante y lo agrega a estructuras.
void Server::acceptClient() {
    // Reserva estructura para direccion del cliente aceptado.
    sockaddr_in clientAddr;
    // Inicializa longitud que accept rellenara con el tamano real.
    socklen_t len = sizeof(clientAddr);

    // Acepta una nueva conexion entrante desde el socket servidor.
    int clientFd = accept(_serverFd, (sockaddr*)&clientAddr, &len);
    // Si no hay cliente valido disponible, termina sin hacer nada.
    if (clientFd < 0)
        return;

    int flags = fcntl(clientFd, F_GETFL, 0); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)
    if (flags >= 0)
        fcntl(clientFd, F_SETFL, flags | O_NONBLOCK); // tiene que ser asi segun eval hub fcntl(fd, F_SETFL, O_NONBLOCK)

    // Convierte IP binaria del cliente a string legible.
    std::string ip = inet_ntoa(clientAddr.sin_addr);

    // Crea objeto Client y lo guarda en la lista de clientes.
    Client* client = new Client(clientFd, ip);
    _clients.push_back(client);

    // Registra su fd en poll para detectar lectura inicial.
    pollfd p;
    p.fd = clientFd;
    p.events = POLLIN;
    _fds.push_back(p);

    // Informa por consola la conexion entrante aceptada.
    std::cout << "New client: " << clientFd << std::endl;
}

// Recibe datos, acumula en buffer y procesa cada linea completa IRC.
void Server::handleClientRead(size_t i) {
    // Buffer temporal para leer bytes desde el socket.
    char buffer[1024];
    // Limpia el buffer para evitar basura en memoria.
    memset(buffer, 0, sizeof(buffer));

    // Lee datos del socket asociado al indice de poll.
    int bytes = recv(_fds[i].fd, buffer, sizeof(buffer), 0);

    // Busca el objeto Client correspondiente al fd.
    Client* client = getClientByFd(_fds[i].fd);
    // Si no existe el cliente, elimina la entrada inconsistente.
    if (!client) {
        disconnectClient(i);
        return;
    }

    // Si recv devuelve 0, el peer cerro la conexion.
    if (bytes == 0) {
        disconnectClient(i);
        return;
    }

    // Maneja error de lectura y distingue error temporal.
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(i);
        return;
    }

    // Agrega los bytes recibidos al buffer de lectura acumulado.
    client->appendReadBuffer(std::string(buffer, bytes));

    // Extrae lineas IRC completas y las manda al CommandHandler.
    std::string line;
    while (client->hasCompleteLine()) {
        line = client->extractLine();
        Message msg(line);
        _handler.execute(*client, msg, _clients);
    }
}

// Envía datos pendientes del buffer de salida y maneja envio parcial.
void Server::handleClientWrite(size_t i) {
    // Obtiene el cliente que corresponde al fd actual.
    Client* client = getClientByFd(_fds[i].fd);
    // Si no existe cliente, desconecta el fd por seguridad.
    if (!client) {
        disconnectClient(i);
        return;
    }

    // Copia el buffer pendiente de salida del cliente.
    std::string data = client->getWriteBuffer();

    // Si no hay nada para enviar, solo verifica desconexion diferida.
    if (data.empty()) {
        if (client->isToBeDisconnected())
            disconnectClient(i);
        return;
    }

    // Envia al socket la mayor cantidad posible de bytes pendientes.
    int sent = send(_fds[i].fd, data.c_str(), data.size(), 0);

    // Si send falla, ignora bloqueo temporal o desconecta en error real.
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(i);
        return;
    }

    // Si envio parcial, conserva solo la parte no enviada.
    if (static_cast<size_t>(sent) < data.size())
        client->setWriteBuffer(data.substr(sent));
    else
        // Si envio completo, limpia el buffer de salida.
        client->setWriteBuffer("");

    // Si se pidio cerrar y ya no hay pendientes, desconecta cliente.
    if (client->isToBeDisconnected() && client->getWriteBuffer().empty())
        disconnectClient(i);
}

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

void Server::disconnectClient(size_t i) {
    // Guarda el fd antes de borrar la entrada en _fds.
    int fd = _fds[i].fd;

    // Busca y elimina el objeto Client asociado al fd.
    for (size_t j = 0; j < _clients.size(); j++) {
        if (_clients[j]->getFd() == fd) {
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            break;
        }
    }

    // Cierra descriptor del socket y lo quita de la lista de poll.
    close(fd);
    _fds.erase(_fds.begin() + i);

    // Registra por consola que el cliente fue desconectado.
    std::cout << "Disconnected: " << fd << std::endl;
}

// Busca un cliente por fd en la lista y devuelve puntero o NULL.
Client* Server::getClientByFd(int fd) {
    // Recorre clientes conectados hasta encontrar fd coincidente.
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i]->getFd() == fd)
            return _clients[i];
    }
    // Si no existe coincidencia, devuelve NULL.
    return NULL;
}