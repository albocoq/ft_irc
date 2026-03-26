#include <string>
#include <map>
#include "Client.hpp"

class Channel
{
    private:
        std::string _name;
        std::map<int, Client*> _clients;
    public: 
        Channel(std::string name);
        ~Channel();

        std::string getNameChannel() const;
        const std::map<int, Client*> &getAllChanel() const;
        void addClient(Client &client);
        void removeClient(int fd);
};

