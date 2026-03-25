#include "CommandHandler.hpp"

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