#include "CommandHandler.hpp"

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
