#include "Channel.hpp"

Channel::Channel(std::string name): _name(name) {}

Channel::~Channel() {}

std::string Channel::getNameChannel() const {
    return _name;
}

const std::map<int, Client*> &Channel::getAllChanel() const {
    return _clients;
}

void Channel::addClient(Client &client) {
    _clients.insert(std::make_pair(client.getFd(), &client));
}

void Channel::removeClient(int fd) {
    _clients.erase(fd);
}