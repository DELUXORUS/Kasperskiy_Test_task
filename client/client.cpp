#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "client.hpp"


Client::Client()
{
    _socket = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket < 0 )
    {
        throw std::runtime_error("[Client::Client] socket() call error");
    }

    sockaddr_in serverAddr{};
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

bool Client::sendFile(std::string pathToFile, int port)
{

    std::ifstream file(pathToFile, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("File opening error");
    }

    char buffer[4096];

    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) 
    {
        size_t bytes = file.gcount();
        send(_socket, buffer, bytes, 0);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    std::string host = "localhost";
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


