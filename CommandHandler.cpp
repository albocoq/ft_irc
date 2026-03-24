#include "CommandHandler.hpp"

CommandHandler::CommandHandler(std::string serverPassword): _serverPassword(serverPassword) {
	_commands.insert(std::make_pair("PASS", &CommandHandler::handlePassword));
	_commands.insert(std::make_pair("NICK", &CommandHandler::handlePseudo));
	_commands.insert(std::make_pair("USER", &CommandHandler::handleUser));
	_commands.insert(std::make_pair("PRIVMSG", &CommandHandler::handlePrivmsg));
	_commands.insert(std::make_pair("PING", &CommandHandler::handlePing));
	_commands.insert(std::make_pair("QUIT", &CommandHandler::handleQuit));
}

void CommandHandler::execute(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::string CommandWord = message.getCommand();

	std::map<std::string, CommandFn>::iterator it = _commands.find(CommandWord);
	if (it != _commands.end())
		(this->*(it->second))(client, message, annular);
	else
		std::cout << "Commande not found" << std::endl;
}

void CommandHandler::handlePassword(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();

	if (params.size() >= 1) {
		if (params.front() == _serverPassword)
			client.setHasPassed(true);
		else
			client.appendWriteBuffer(":ircserv 464 " + client.getNickname() + " :Password incorrect");
	} else
		client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " password :Not enough parameters");
}

void CommandHandler::handlePseudo(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();

	if (params.size() >= 1) {
		if (!params.front().empty()) {
			std::string pseudo = params.front();

			for (size_t i = 0; i < annular.size(); i++) {
				if (annular[i]->getNickname() == pseudo && annular[i] != &client) {
					client.appendWriteBuffer(":ircserv 433 * " + pseudo + " :Nickname is already in use");
					return;
				}
			}
			client.setHasNickname(true);
			client.setNickname(pseudo);
			checkRegistration(client);
		}
	} else
		client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " pseudo :Not enough parameters");
}

void CommandHandler::handleUser(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();

	if (params.size() >= 1) {
		client.setUsername(params.front());
		client.setRealname(params.back());
		client.setHasUser(true);
		checkRegistration(client);
	} else
		client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " user :Not enough parameters");
}

void CommandHandler::checkRegistration(Client& client) {
	if (client.isRegistered())
		return;

	if (client.hasUser() && client.hasNickname() && client.hasPassed()) {
		client.setRegistered(true);
		client.appendWriteBuffer(":ircserv 001 " + client.getNickname() + " :Welcome to the ft_irc network!");
	}
}

void CommandHandler::handlePrivmsg(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();

	if (params.size() < 2) {
		client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " privmsg :Not enough parameters");
		return;
	}

	std::string recipient = params.front();
	std::string msgText = params.back();
	if (recipient[0] == '#') {
		// TODO: Trabajo de persona 3
	} else {
		for (size_t i = 0; i < annular.size(); i++) {
			if (annular[i]->getNickname() == recipient) {
				annular[i]->appendWriteBuffer(":" + client.getNickname() + " PRIVMSG " + recipient + " :" + msgText);
				return;
			}
		}
		client.appendWriteBuffer(":ircserv 401 " + client.getNickname() + " " + recipient + " :No such nick/channel");
	}
}

void CommandHandler::handlePing(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();
	
	if (params.size() < 1) {
		client.appendWriteBuffer(":ircserv 409 " + client.getNickname() + " :No origin specified");        
	} else {
		client.appendWriteBuffer(":ircserv PONG ircserv :" + params.front());
	}
}

void CommandHandler::handleQuit(Client& client, const Message& message, std::vector<Client*>& annular) {
	std::vector<std::string> params = message.getParameters();
	std::string quitMsg;

	if (params.size() >= 1)
		quitMsg = params.front();
	// Tel all servers thax to person 3
	
	client.appendWriteBuffer("ERROR :Closing Link: " + client.getIp() + " (Quit)");
	client.setToBeDisconnected(true);
}

void CommandHandler::handleJoin(Client &client, const Message &message, std::vector<Client*>&clients){
	std::vector<std::string> params = message.getParameters();
	if (!params.empty())
	{
		std::string channelName = params[0];

		Channel *channel = NULL;
		std::map<std::string, Channel*>::iterator it = _channels.find(channelName);
		if (it == _channels.end())
		{
			channel = new Channel(channelName);
			_channels[channelName] = channel;
			channel->addOperator(&client);
		}
		else
			channel = it->second;
		channel->addMember(&client);
		client.appendWriteBuffer(":server JOIN " + channelName + "\r\n");
	}
	else
	client.appendWriteBuffer(":ircserv 461 " + client.getNickname() + " join :Not enough parameters");
}
