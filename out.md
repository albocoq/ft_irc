#include "Client.hpp"

Client::Client(int fd, std::string ip) : _fd(fd), _ip(ip) {
    _isRegistered = false;
    _hasPassed = false;
    _hasNickname = false;
    _hasUser = false;
    _toDisconnected = false;

    _readBuffer.clear();
    _writeBuffer.clear();
}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

std::string Client::getIp() const {
    return _ip;
}

std::string Client::getReadBuffer() const {
    return _readBuffer;
}

std::string Client::getWriteBuffer() const {
    return _writeBuffer;
}

std::string Client::getNickname() const {
    return _nickname;
}

std::string Client::getUsername() const {
    return _username;
}

std::string Client::getRealname() const {
    return _realname;
}

bool Client::isRegistered() const {
    return _isRegistered;
}

bool Client::hasPassed() const {
    return _hasPassed;
}

bool Client::hasNickname() const {
    return _hasNickname;
}

bool Client::hasUser() const {
    return _hasUser;
}

bool Client::isToBeDisconnected() const {
    return _toDisconnected;
}

void Client::setReadBuffer(const std::string& buffer) {
    _readBuffer = buffer;
}

void Client::setWriteBuffer(const std::string& buffer) {
    _writeBuffer = buffer;
}

void Client::setNickname(const std::string& nickname) {
    _nickname = nickname;
}

void Client::setUsername(const std::string& username) {
    _username = username;
}

void Client::setRealname(const std::string& realname) {
    _realname = realname;
}

void Client::setRegistered(bool isRegistered) {
    _isRegistered = isRegistered;
}

void Client::setHasPassed(bool hasPassed) {
    _hasPassed = hasPassed;
}

void Client::setHasNickname(bool hasNickname) {
    _hasNickname = hasNickname;
}

void Client::setHasUser(bool hasUser) {
    _hasUser = hasUser;
}

void Client::setToBeDisconnected(bool status) {
    _toDisconnected = status;
}

std::string Client::extractLine() {
    size_t pos = _readBuffer.find("\r\n");

    if (pos == std::string::npos)
        return "";

    std::string line = _readBuffer.substr(0, pos);

    _readBuffer.erase(0, pos + 2);
    
    return line;
}

void Client::appendReadBuffer(std::string data) {
    _readBuffer.append(data);
}

void Client::appendWriteBuffer(std::string message) {
    
    if (message.length() >= 2 && message[message.length() - 1] != '\n' && message[message.length() - 2] != '\r')
        message.append("\r\n");
    
    _writeBuffer.append(message);
}#ifndef CLIENT
#define CLIENT

#include <string>

class Client {
    private:
        int _fd;
        std::string _ip;
        std::string _readBuffer;
        std::string _writeBuffer;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        bool _isRegistered;
        bool _hasPassed;
        bool _hasNickname;
        bool _hasUser;
        bool _toDisconnected;

    public:
        Client(int fd, std::string ip);
        ~Client();
        int getFd() const;
        std::string getIp() const;
        std::string getReadBuffer() const;
        std::string getWriteBuffer() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        bool isRegistered() const;
        bool hasPassed() const;
        bool hasNickname() const;
        bool hasUser() const;
        bool isToBeDisconnected() const;
        void setReadBuffer(const std::string& buffer);
        void setWriteBuffer(const std::string& buffer);
        void setNickname(const std::string& nickname);
        void setUsername(const std::string& username);
        void setRealname(const std::string& realname);
        void setRegistered(bool isRegistered);
        void setHasPassed(bool hasPassed);
        void setHasNickname(bool hasNickname);
        void setHasUser(bool hasUser);
        void setToBeDisconnected(bool status);
        void appendReadBuffer(std::string data);
        void appendWriteBuffer(std::string message);
        std::string extractLine();
};

#endif#include "CommandHandler.hpp"

CommandHandler::CommandHandler(std::string serverPassword): _serverPassword(serverPassword) {
    _commands.insert(std::make_pair("PASS", &CommandHandler::handlePassword));
    _commands.insert(std::make_pair("NICK", &CommandHandler::handlePseudo));
    _commands.insert(std::make_pair("USER", &CommandHandler::handleUser));
    _commands.insert(std::make_pair("PRIVMSG", &CommandHandler::handlePrivmsg));
    _commands.insert(std::make_pair("PING", &CommandHandler::handlePing));
    _commands.insert(std::make_pair("QUIT", &CommandHandler::handleQuit));
}

void CommandHandler::execute(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::string CommandWord = message.getCommand();

    std::map<std::string, CommandFn>::iterator it = _commands.find(CommandWord);
    if (it != _commands.end())
        (this->*(it->second))(client, message, annular);
    else
        std::cout << "Commande not found" << std::endl;
}

void CommandHandler::handlePassword(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();

    if (params.size() >= 1) {
        if (params.front() == _serverPassword)
            client.setHasPassed(true);
        else
            client.appendWriteBuffer(":ircserv 464 " + client.getNickname() + " :Password incorrect");
    } else
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " password :Not enough parameters");
}

void CommandHandler::handlePseudo(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();

    if (params.size() >= 1) {
        if (!params.front().empty()) {
            std::string pseudo = params.front();

            for (size_t i = 0; i < annular.size(); i++) {
                if (annular[i]->getNickname() == pseudo && annular[i] != &client) {
                    client.appendWriteBuffer(":ircserv 433 * " + pseudo + " :Nickname is already in use");
                    return;
                }
            }
            client.setHasNickname(true);
            client.setNickname(pseudo);
            checkRegistration(client);
        }
    } else
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " pseudo :Not enough parameters");
}

void CommandHandler::handleUser(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();

    if (params.size() >= 1) {
        client.setUsername(params.front());
        client.setRealname(params.back());
        client.setHasUser(true);
        checkRegistration(client);
    } else
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " user :Not enough parameters");
}

void CommandHandler::checkRegistration(Client& client) {
    if (client.isRegistered())
        return;

    if (client.hasUser() && client.hasNickname() && client.hasPassed()) {
        client.setRegistered(true);
        client.appendWriteBuffer(":ircserv 001 " + client.getNickname() + " :Welcome to the ft_irc network!");
    }
}

void CommandHandler::handlePrivmsg(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();

    if (params.size() < 2) {
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " privmsg :Not enough parameters");
        return;
    }

    std::string recipient = params.front();
    std::string msgText = params.back();
    if (recipient[0] == '#') {
        // TODO: Trabajo de persona 3
    } else {
        for (size_t i = 0; i < annular.size(); i++) {
            if (annular[i]->getNickname() == recipient) {
                annular[i]->appendWriteBuffer(":" + client.getNickname() + " PRIVMSG " + recipient + " :" + msgText);
                return;
            }
        }
        client.appendWriteBuffer(":ircserv 401 " + client.getNickname() + " " + recipient + " :No such nick/channel");
    }
}

void CommandHandler::handlePing(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();
    
    if (params.size() < 1) {
        client.appendWriteBuffer(":ircserv 409 " + client.getNickname() + " :No origin specified");        
    } else {
        client.appendWriteBuffer(":ircserv PONG ircserv :" + params.front());
    }
}

void CommandHandler::handleQuit(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::vector<std::string> params = message.getParameters();
    std::string quitMsg;

    if (params.size() >= 1)
        quitMsg = params.front();
    // Tel all servers thax to person 3
    
    client.appendWriteBuffer("ERROR :Closing Link: " + client.getIp() + " (Quit)");
    client.setToBeDisconnected(true);
}
#ifndef COMMANDHANDLER
#define COMMANDHANDLER

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "Client.hpp"
#include "Message.hpp"

class CommandHandler {
    private:
        typedef void (CommandHandler::*CommandFn)(Client&, const Message&, std::vector<Client*>&);
        std::map<std::string, CommandFn> _commands;
        std::string _serverPassword;

        void handlePassword(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePseudo(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleUser(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePrivmsg(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePing(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleQuit(Client& client, const Message& message, std::vector<Client*>& annular);

        void checkRegistration(Client& client);

    public:
        CommandHandler(std::string serverPassword);
        ~CommandHandler();

        void execute(Client& client, const Message& message, std::vector<Client*>& annular);

};

#endif#include "Message.hpp"

// Prefix Command Params
// :Zelda PRIVMSG Link :Salut, comment ça va ?

Message::Message(const std::string& rawLine) {
    std::string copyRawLine = rawLine;

    if (rawLine.empty()) {
        setCommand("");
        return;
    }

    if (copyRawLine[0] == ':') {
        setPrefix(copyRawLine.substr(1, copyRawLine.find(" ") - 1));
        copyRawLine.erase(0, copyRawLine.find(" ") + 1);
    }

    if (copyRawLine.find(" ") == std::string::npos) {
        setCommand(copyRawLine);
        copyRawLine.clear();
        return;
    }
    setCommand(copyRawLine.substr(0, copyRawLine.find(" ")));
    copyRawLine.erase(0, copyRawLine.find(" ") + 1);
    
    setParameters(copyRawLine);
}

Message::~Message() {}

std::string Message::getPrefix() const {
    return _prefix;
}

std::string Message::getCommand() const {
    return _command;
}

std::vector<std::string> Message::getParameters() const {
    return _parameters;
}

void Message::setPrefix(const std::string& prefix) {
    _prefix = prefix;
}

void Message::setCommand(const std::string& command) {
    _command = command;
}

void Message::setParameters(const std::string& parameters) {
    _parameters.clear();

    std::string copyParameters = parameters;

    while (copyParameters != "") {
        size_t posSpace = copyParameters.find(" ");
        size_t posDoublePoint = copyParameters.find(":");

        if (posSpace < posDoublePoint && posSpace != std::string::npos) {
            _parameters.push_back(copyParameters.substr(0, posSpace));
            copyParameters.erase(0, posSpace + 1);
        } else {
            if (copyParameters[0] == ':')
                _parameters.push_back(copyParameters.substr(1));
            else
                _parameters.push_back(copyParameters);
            copyParameters.clear();
        }
    }
}
#ifndef MESSAGE
#define MESSAGE

#include <string>
#include <vector>

class Message {
    private:
        std::string _prefix;
        std::string _command;
        std::vector<std::string> _parameters;

    public:
        Message(const std::string& rawLine);
        ~Message();
        std::string getPrefix() const;
        std::string getCommand() const;
        std::vector<std::string> getParameters() const;
        void setPrefix(const std::string& prefix);
        void setCommand(const std::string& command);
        void setParameters(const std::string& parameters);
};



## 🌐 Persona A: Arquitecto de Red (Sockets y Multiplexación)

**Objetivo:** Construir el puente de comunicación entre Internet y el Motor Core. No tocarás la lógica de los comandos IRC, solo el flujo de bytes.

* **Apertura del Servidor:** Crear el "Socket" principal, vincularlo a un puerto de red (`bind`) y ponerlo a escuchar conexiones entrantes (`listen`).
* **Multiplexación (El corazón de la red):** Implementar `epoll`, `poll` o `select`. El objetivo es tener un solo bucle capaz de vigilar decenas de conexiones simultáneamente sin bloquear el programa.
* **I/O No Bloqueante:** Configurar todos los File Descriptors (FDs) con la opción `O_NONBLOCK` usando la función `fcntl` (Requisito estricto de 42).
* **Recepción (Read):** Cuando el multiplexor detecte actividad entrante, usar `recv()` para capturar el texto en bruto e inyectarlo en el motor usando: `cliente->appendReadBuffer(datos)`.
* **Envío (Write):** Comprobar constantemente si el cliente tiene respuestas pendientes. Si `cliente->getWriteBuffer()` no está vacío, usar `send()` para enviarlo por red y luego vaciar el búfer.
* **Gestión de Desconexión:** Vigilar el método `cliente->isToBeDisconnected()`. Si devuelve `true` (ej. tras un comando `QUIT`), cerrar la conexión de red con `close(fd)` y destruir el objeto de forma segura.

---

## 🏠 Persona B: Arquitecto de Salas (Gestión de Canales)

**Objetivo:** Apoyarse en el enrutador actual (`CommandHandler`) para crear y gestionar el concepto de "Salas de chat" (canales que empiezan por `#`).

* **La Clase `Channel`:** Crear este nuevo objeto. Debe contener el nombre de la sala y una lista de los usuarios que están dentro (ej. un `std::vector<Client*>`).
* **Directorio de Salas:** Añadir un `std::map` global en el servidor o en el enrutador para almacenar y buscar rápidamente las salas activas.
* **Comando `JOIN`:** * Si la sala no existe, crearla y añadir al usuario. 
  * Si existe, añadir al usuario. 
  * Enviar mensaje a los presentes avisando de la entrada y responder al nuevo usuario con la lista de miembros (`353 RPL_NAMREPLY`).
* **Comando `PART`:** Retirar al usuario de la lista de la sala. Si la sala se queda completamente vacía, destruir el objeto `Channel` para liberar memoria.
* **Actualizar `PRIVMSG`:** Modificar la función `handlePrivmsg` existente. Cuando el objetivo empiece por `#`, buscar la sala e iterar para copiar el mensaje en el `_writeBuffer` de **todos** los miembros (excepto el remitente).

---

## 🛡️ Persona C: Moderador (Privilegios y Comandos Avanzados)

**Objetivo:** Implementar las reglas de administración y los poderes especiales dentro de las salas creadas por la Persona B.

* **Estado de Operador:** Actualizar la clase `Channel` para diferenciar a un usuario normal de un operador (usualmente indicado con un `@` en su apodo).
* **Comando `KICK`:** Función para expulsar a un usuario de una sala. Requiere verificar primero si el emisor tiene privilegios de operador en esa sala específica.
* **Comando `TOPIC`:** Permitir a los usuarios consultar el tema actual de la sala, o modificarlo si tienen permiso.
* **Comando `INVITE`:** Permitir invitar a un usuario específico (que está en el servidor pero no en la sala) a unirse a un canal privado.
* **Comando `MODE`:** El comando más complejo. Permite cambiar la configuración de la sala. Se deben gestionar 5 modos:
  * `+i`: Establecer la sala como "Solo por invitación".
  * `+t`: Restringir el derecho de cambiar el `TOPIC` solo a los operadores.
  * `+k`: Añadir una contraseña para poder entrar a la sala.
  * `+o`: Dar (o quitar) privilegios de operador a otro miembro.
  * `+l`: Definir un límite máximo de usuarios simultáneos en la sala.

---
*Fin del documento.*#include "Server.hpp"
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
}#ifndef SERVER_HPP
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
   

private:
    
};

#endif## 🌐 Persona A: Arquitecto de Red (Sockets y Multiplexación)

**Objetivo:** Construir el puente de comunicación entre Internet y el Motor Core. No tocarás la lógica de los comandos IRC, solo el flujo de bytes.

* **Apertura del Servidor:** Crear el "Socket" principal, vincularlo a un puerto de red (`bind`) y ponerlo a escuchar conexiones entrantes (`listen`).
* **Multiplexación (El corazón de la red):** Implementar `epoll`, `poll` o `select`. El objetivo es tener un solo bucle capaz de vigilar decenas de conexiones simultáneamente sin bloquear el programa.
* **I/O No Bloqueante:** Configurar todos los File Descriptors (FDs) con la opción `O_NONBLOCK` usando la función `fcntl` (Requisito estricto de 42).
* **Recepción (Read):** Cuando el multiplexor detecte actividad entrante, usar `recv()` para capturar el texto en bruto e inyectarlo en el motor usando: `cliente->appendReadBuffer(datos)`.
* **Envío (Write):** Comprobar constantemente si el cliente tiene respuestas pendientes. Si `cliente->getWriteBuffer()` no está vacío, usar `send()` para enviarlo por red y luego vaciar el búfer.
* **Gestión de Desconexión:** Vigilar el método `cliente->isToBeDisconnected()`. Si devuelve `true` (ej. tras un comando `QUIT`), cerrar la conexión de red con `close(fd)` y destruir el objeto de forma segura.