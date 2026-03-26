#include "Channel.hpp"
#include "Client.hpp"

void handleModeTopicRestricted(Channel* channel, bool set, Client& client) {
    channel->topicRestricted = set;
    client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + (set ? " +t" : " -t"));
}
