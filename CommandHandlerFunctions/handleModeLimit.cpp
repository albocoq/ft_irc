#include "Channel.hpp"
#include "Client.hpp"

void handleModeLimit(Channel* channel, bool set, Client& client, int limit) {
    if (set && limit > 0) {
        channel->userLimit = limit;
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " +l " + std::to_string(limit));
    } else {
        channel->userLimit = -1;
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " -l");
    }
}
