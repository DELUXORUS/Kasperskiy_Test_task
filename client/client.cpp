#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>
#include <iostream>
#include <netdb.h>

#include "client.hpp"


Client::Client()
{
    _socket = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket < 0 )
    {
        throw std::runtime_error("[Client::Client] socket() call error");
    }

    sockaddr_in serverAddr;
    memset(reinterpret_cast<char*>(&serverAddr), '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = INADDR_ANY;

    if (bind(_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) != 0)
    {
        throw std::runtime_error("[Client::Client] bind() call error");
    }
    
    std::cout << "[Client::Client] Ready" << std::endl;
}

Client::~Client()
{
    shutdown(_socket, 0);
}

bool Client::sendFile(std::string host, int port)
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    hostent* he  = gethostbyname(host.c_str());

    if (!he)
    {
        std::cerr << "Host not found\n";
        return false;
    }

    memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection error!\n" << std::endl;
        return false;
    }

    std::cout << "Connection to server complete!" << std::endl;
    return 1;
}


