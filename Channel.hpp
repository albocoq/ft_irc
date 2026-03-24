#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>
#include "Client.hpp"

class Channel {
public:
	explicit Channel(const std::string &name = "");

	const std::string &getName() const;

	void addMember(Client *client);
	void removeMember(Client *client);
	bool hasMember(Client *client) const;
	bool hasMemberByFd(int fd) const;

	void addOperator(Client *client);
	void removeOperator(Client *client);
	bool isOperator(Client *client) const;

	void invite(Client *client);
	void uninvite(Client *client);
	bool isInvited(Client *client) const;

	bool empty() const;
	size_t memberCount() const;

	std::string topic;
	bool inviteOnly;
	bool topicRestricted;
	std::string key;
	int userLimit;

	std::set<Client *> members;
	std::set<Client *> operators;
	std::set<Client *> invited;

private:
	std::string _name;
};

#endif
