#include "CommandHandler.hpp"

CommandHandler::CommandHandler(std::string serverPassword): _serverPassword(serverPassword) {
    _commands.insert(std::make_pair("PASS", &CommandHandler::handlePassword));
    _commands.insert(std::make_pair("NICK", &CommandHandler::handlePseudo));
    _commands.insert(std::make_pair("USER", &CommandHandler::handleUser));
    _commands.insert(std::make_pair("PRIVMSG", &CommandHandler::handlePrivmsg));
    _commands.insert(std::make_pair("PING", &CommandHandler::handlePing));
    _commands.insert(std::make_pair("QUIT", &CommandHandler::handleQuit));
    _commands.insert(std::make_pair("JOIN", &CommandHandler::handleJoin));
    _commands.insert(std::make_pair("PART", &CommandHandler::handlePart));
}

void CommandHandler::execute(Client& client, const Message& message, std::vector<Client*>& annular) {
    std::string CommandWord = message.getCommand();

    std::map<std::string, CommandFn>::iterator it = _commands.find(CommandWord);
    if (it != _commands.end())
        (this->*(it->second))(client, message, annular);
    else
        std::cout << "Commande not found" << std::endl;
}

void CommandHandler::checkRegistration(Client& client) {
    if (client.isRegistered())
        return;

    if (client.hasUser() && client.hasNickname() && client.hasPassed()) {
        client.setRegistered(true);
        client.appendWriteBuffer(":ircserv 001 " + client.getNickname() + " :Welcome to the ft_irc network!");
    }
}
