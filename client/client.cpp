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
    
    std::cout << "[Client::Client] Ready" << std::endl;
}

Client::~Client()
{
    close(_socket);
}

bool Client::sendFile(std::string pathToFile, int port)
{

    std::ifstream file(pathToFile, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("File opening error");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    std::string host = "localhost";
    hostent* he  = gethostbyname(host.c_str());

    if (!he)
    {
        std::cerr << "Host not found" << std::endl;
        return false;
    }

    memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        std::cerr << "Connection error!" << std::endl;
        return false;
    }

    char buffer[4096];
    while (true)
    {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();

        if (bytesRead < 0)
        {
            std::cerr << "File read error" << std::endl;
            return false;
        }

        if (bytesRead == 0)
            break;

        std::streamsize totalSent = 0;
        while (totalSent < bytesRead)
        {
            ssize_t sent = send(_socket, buffer + totalSent, bytesRead - totalSent, 0);

            if (sent < 0)
            {
                if (errno == EINTR)
                    continue;

                std::cerr << "[Client::sendFile] send() error" << std::endl;
                return false;
            }

            totalSent += sent;
        }

        if (file.eof())
            break;
    }

    

    std::cout << "The data has been transmitted" << std::endl;

    shutdown(_socket, SHUT_WR);

    std::cout << "[Client::sendFile] Waiting for server response..." << std::endl;

    char recvBuffer[4096];

    while (true)
    {
        ssize_t bytes = recv(_socket, recvBuffer, sizeof(recvBuffer), 0);

        if (bytes == 0)
        {
            break;
        }
        else if (bytes < 0)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "[Client::sendFile] recv() error" << std::endl;
            return false;
        }

        std::cout.write(recvBuffer, bytes);
    }

    std::cout << std::endl;
    return 1;
}


