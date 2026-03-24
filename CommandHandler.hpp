#ifndef COMMANDHANDLER
#define COMMANDHANDLER

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "Client.hpp"
#include "Message.hpp"
#include "Channel.hpp"

class CommandHandler {
	private:
		typedef void (CommandHandler::*CommandFn)(Client&, const Message&, std::vector<Client*>&);
		std::map<std::string, CommandFn> _commands;
		std::string _serverPassword;
		std::map<std::string, Channel*> _channels;

		void handlePassword(Client& client, const Message& message, std::vector<Client*>& annular);
		void handlePseudo(Client& client, const Message& message, std::vector<Client*>& annular);
		void handleUser(Client& client, const Message& message, std::vector<Client*>& annular);
		void handlePrivmsg(Client& client, const Message& message, std::vector<Client*>& annular);
		void handlePing(Client& client, const Message& message, std::vector<Client*>& annular);
		void handleQuit(Client& client, const Message& message, std::vector<Client*>& annular);
		void handleJoin(Client &client, const Message &message, std::vector<Client *> &clients);

		void checkRegistration(Client& client);

	public:
		CommandHandler(std::string serverPassword);
		~CommandHandler();

		void execute(Client& client, const Message& message, std::vector<Client*>& annular);

};

#endif