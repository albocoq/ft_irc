#include "../Channel.hpp"
#include "../Client.hpp"

void handleModeInviteOnly(Channel* channel, bool set, Client& client) {
    channel->inviteOnly = set;
    client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + (set ? " +i" : " -i"));
}
