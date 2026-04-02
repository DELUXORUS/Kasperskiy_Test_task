#ifndef SERVER_HPP_
#define SERVER_HPP_


#include <string>
#include <unordered_set>


class Server
{
    public:
        Server(int port) : _port(port) { _setupSocket(); };

        void run();

        ~Server();
    private:
        int _socket;
        int _port;
        std::unordered_set<pid_t> _childs;

        void _setupSocket();
        void _handleClient(int clientSocket);
        void _removeZombie();
        void _shutdown();
};


#endif