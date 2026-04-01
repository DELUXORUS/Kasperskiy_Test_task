#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <string>


class Client
{
    public:
        Client();

        bool sendFile(std::string, int port);

        ~Client();
    private:
        int _socket;
};


#endif