#include "../CommandHandler.hpp"

void CommandHandler::handlePassword(Client& client, const Message& message, std::vector<Client*>& annular) {
    (void)annular;
    std::vector<std::string> params = message.getParameters();

    if (params.size() >= 1) {
        if (params.front() == _serverPassword)
            client.setHasPassed(true);
        else
            client.appendWriteBuffer(":ircserv 464 " + client.getNickname() + " :Password incorrect");
    } else
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " password :Not enough parameters");
}
