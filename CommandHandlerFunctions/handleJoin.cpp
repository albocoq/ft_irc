#include "../CommandHandler.hpp"

void CommandHandler::handleJoin(Client& client, const Message& message, std::vector<Client*>& annular) {
    (void)annular;
    std::vector<std::string> params = message.getParameters();
    
    if (params.size() < 1) {
        client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " join :Not enough parameters");
        return;
    }

    std::string channelName = params.front();
    
    std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
    Channel* currentChannel;

    if (it != _channels.end()) {
        currentChannel = it->second;
        it->second->addClient(client);
    } else {
        currentChannel = new Channel(channelName);

        currentChannel->addOperator(&client);
        currentChannel->addClient(client);
        _channels.insert(std::make_pair(channelName, currentChannel));
    }

    std::map<int, Client*> allClients = currentChannel->getAllChanel();
    std::map<int, Client*>::const_iterator firstClient = allClients.begin();
    std::map<int, Client*>::const_iterator lastClient = allClients.end();
    std::string listNicks = "";

    while (firstClient != lastClient) {
        firstClient->second->appendWriteBuffer(":" + client.getNickname() + " JOIN " + channelName);
        listNicks += firstClient->second->getNickname() + " ";
        firstClient++;
    }
    

    client.appendWriteBuffer(":ircserv 353 " + client.getNickname() + " = " + currentChannel->getNameChannel() + " :" + listNicks);
    client.appendWriteBuffer(":ircserv 366 " + client.getNickname() + " " + currentChannel->getNameChannel() + " :End of /NAMES list");
}