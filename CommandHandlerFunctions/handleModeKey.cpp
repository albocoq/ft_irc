#include "Channel.hpp"
#include "Client.hpp"

void handleModeKey(Channel* channel, bool set, Client& client, const std::string& key) {
    if (set) {
        channel->key = key;
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " +k");
    } else {
        channel->key.clear();
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " -k");
    }
}
