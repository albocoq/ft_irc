#include "../CommandHandler.hpp"

void CommandHandler::handlePing(Client& client, const Message& message, std::vector<Client*>& annular) {
    (void)annular;
    std::vector<std::string> params = message.getParameters();
    
    if (params.size() < 1) {
        client.appendWriteBuffer(":ircserv 409 " + client.getNickname() + " :No origin specified");        
    } else {
        client.appendWriteBuffer(":ircserv PONG ircserv :" + params.front());
    }
}