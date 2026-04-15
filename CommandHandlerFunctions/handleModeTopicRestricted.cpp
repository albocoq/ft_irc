#include "../Channel.hpp"
#include "../Client.hpp"

void handleModeTopicRestricted(Channel* channel, bool set, Client& client) {
    channel->topicRestricted = set;
    client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + (set ? " +t" : " -t") + "\033[0m");
}
