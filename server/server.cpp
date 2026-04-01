#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <signal.h>

#include "server.hpp"


Server::~Server()
{
    if (_socket >= 0)
    {
        close(_socket);
    }
}

void Server::_setupSocket()
{
    _socket = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket < 0)
    {
        throw std::runtime_error("[Server::_setupSocket] socket() error");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(_port);

    if (bind(_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
        close(_socket);
        throw std::runtime_error("[Server::_setupSocket] bind() error");
    }

    if (listen(_socket, SOMAXCONN) < 0)
    {
        close(_socket);
        throw std::runtime_error("[Server::_setupSocket] listen() error");
    }

    std::cout << "[Server::_setupSocket] Listening on port " << _port << std::endl;
}

void Server::_handleClient(int clientSocket)
{
    
}

void Server::run()
{
    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(
            _socket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrLen
        );

        if (clientSocket < 0)
        {
            std::cerr << "[Server::run] accept() error" << std::endl;
            continue;
        }

        std::cout << "[Server::run] New connection from "
                  << inet_ntoa(clientAddr.sin_addr)
                  << ":" << ntohs(clientAddr.sin_port)
                  << std::endl;

        pid_t pid = fork();

        if (pid < 0)
        {
            std::cerr << "[Server::run] fork() error" << std::endl;
            close(clientSocket);
            continue;
        }

        if (pid == 0)
        {
            close(_socket);
            _handleClient(clientSocket);
            _exit(0);
        }
        else
        {
            close(clientSocket);
        }
    }
}



