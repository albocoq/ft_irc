#include "../CommandHandler.hpp"

void CommandHandler::handleWho(Client& client, const Message& message, std::vector<Client*>& annular) {
    (void)annular;
    std::vector<std::string> params = message.getParameters();

    if (params.size() < 1) {
        client.appendWriteBuffer(redMessage(":ircserv 461 " + client.getNickname() + " who :Not enough parameters"));
        return;
    }

    std::string channelName = params.front();

    if (channelName[0] == '#') {
        std::map<std::string, Channel*>::iterator it = _channels.find(channelName);

        if (it != _channels.end()) {
            std::map<int, Client*>::const_iterator firstClient = it->second->getAllChanel().begin();
            std::map<int, Client*>::const_iterator lastClient = it->second->getAllChanel().end();

            while (firstClient != lastClient)
            {
                client.appendWriteBuffer(":ircserv 352 " + client.getNickname() + " " + channelName + " " +  firstClient->second->getUsername() + " " + firstClient->second->getIp() + " ircserv " + firstClient->second->getNickname() + " H :0 " + firstClient->second->getRealname());

                firstClient++;
            }
        } else
            client.appendWriteBuffer(redMessage(":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel"));
    }
    client.appendWriteBuffer(":ircserv 315 " +  client.getNickname() + " " + channelName + " :End of /WHO list");
}