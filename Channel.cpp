#include "Channel.hpp"

Channel::Channel(const std::string &name)
	: inviteOnly(false), topicRestricted(true), userLimit(-1), _name(name) {}

const std::string &Channel::getName() const { return _name; }

void Channel::addMember(Client *client) { members.insert(client); }

void Channel::removeMember(Client *client) {
	members.erase(client);
	operators.erase(client);
	invited.erase(client);
}

bool Channel::hasMember(Client *client) const { return members.find(client) != members.end(); }

void Channel::addOperator(Client *client) { operators.insert(client); }

void Channel::removeOperator(Client *client) { operators.erase(client); }

bool Channel::isOperator(Client *client) const { return operators.find(client) != operators.end(); }

void Channel::invite(Client *client) { invited.insert(client); }

void Channel::uninvite(Client *client) { invited.erase(client); }

bool Channel::isInvited(Client *client) const { return invited.find(client) != invited.end(); }
bool Channel::hasMemberByFd(int fd) const {
	for (std::set<Client *>::const_iterator it = members.begin(); it != members.end(); ++it) {
		if ((*it)->getFd() == fd)
			return true;
	}
	return false;
}

bool Channel::empty() const { return members.empty(); }

size_t Channel::memberCount() const { return members.size(); }
