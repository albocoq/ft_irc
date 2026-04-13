#include "../Channel.hpp"
#include "../Client.hpp"
#include <sstream>

void handleModeLimit(Channel* channel, bool set, Client& client, int limit) {
    if (set && limit > 0) {
        channel->userLimit = limit;
        std::ostringstream oss;
        oss << limit;
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " +l " + oss.str());
    } else {
        channel->userLimit = -1;
        client.appendWriteBuffer(":ircserv MODE " + channel->getNameChannel() + " -l");
    }
}
