#include "../Channel.hpp"
#include "../Client.hpp"
#include <vector>

void handleModeOperator(Channel* channel, bool set, Client& client, const std::string& targetNick, std::vector<Client*>& annular) {
    (void)annular;
    Client* target = NULL;
    std::map<int, Client*> members = channel->getAllChanel();
    
    for (std::map<int, Client*>::iterator mit = members.begin(); mit != members.end(); ++mit) {
        if (mit->second->getNickname() == targetNick) {
            target = mit->second;
            break;
        }
    }
    if (!target) {
        client.appendWriteBuffer("\033[31m:ircserv 441 " + client.getNickname() + " " + targetNick + " " + channel->getNameChannel() + " :They aren't on that channel\033[0m");
        return;
    }
    if (set) {
        channel->addOperator(target);
        client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + " +o " + targetNick + "\033[0m");
    } else {
        channel->removeOperator(target);
        client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + " -o " + targetNick + "\033[0m");
    }
}
