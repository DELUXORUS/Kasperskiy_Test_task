#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <string>


class Client
{
    public:
        Client();

        bool sendFile(std::string pathToFile, int port);

        ~Client();
    private:
        int _socket;
};


#endif