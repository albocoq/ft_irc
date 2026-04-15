#include "../Channel.hpp"
#include "../Client.hpp"

void handleModeKey(Channel* channel, bool set, Client& client, const std::string& key) {
    if (set) {
        channel->key = key;
        client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + " +k\033[0m");
    } else {
        channel->key.clear();
        client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + " -k\033[0m");
    }
}
