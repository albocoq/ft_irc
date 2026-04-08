#ifndef COMMANDHANDLER
#define COMMANDHANDLER

#include <map>
#include <vector>
#include <string>
#include <unistd.h>
#include <iostream>
#include "Client.hpp"
#include "Message.hpp"
#include "Channel.hpp"

class CommandHandler {
    private:
        typedef void (CommandHandler::*CommandFn)(Client&, const Message&, std::vector<Client*>&);
        std::map<std::string, CommandFn> _commands;
        std::string _serverPassword;
        std::map<std::string, Channel*> _channels;

        std::string colorMessage(const std::string& message, const std::string& color) const;
        std::string redMessage(const std::string& message) const;
        std::string greenMessage(const std::string& message) const;
        std::string blueMessage(const std::string& message) const;

        void handlePassword(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePseudo(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleUser(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePrivmsg(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePing(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleQuit(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleJoin(Client& client, const Message& message, std::vector<Client*>& annular);
        void handlePart(Client& client, const Message& message, std::vector<Client*>& annular);
        void handleWho(Client& client, const Message& message, std::vector<Client*>& annular);

        void checkRegistration(Client& client);

    public:
        CommandHandler(std::string serverPassword);
        ~CommandHandler();

        void execute(Client& client, const Message& message, std::vector<Client*>& annular);

};

#endif