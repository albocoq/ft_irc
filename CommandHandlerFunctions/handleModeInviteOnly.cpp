#include "../Channel.hpp"
#include "../Client.hpp"

void handleModeInviteOnly(Channel* channel, bool set, Client& client) {
	channel->inviteOnly = set;
	client.appendWriteBuffer("\033[32m:ircserv MODE " + channel->getNameChannel() + (set ? " +i" : " -i") + "\033[0m");
}
