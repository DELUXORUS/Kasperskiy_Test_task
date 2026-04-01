#include <sys/socket.h>
#include <sys/wait.h>
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

void sigchldHandler(int)
{
    while (waitpid(-1, nullptr, WNOHANG) > 0);
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

    struct sigaction sa;
    sa.sa_handler = sigchldHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, nullptr) < 0)
    {
        close(_socket);
        throw std::runtime_error("sigaction error");
    }

    std::cout << "[Server::_setupSocket] Listening on port " << _port << std::endl;
}

void Server::_handleClient(int clientSocket)
{
    
}

void Server::run()
{
    signal(SIGINT, SIG_DFL);
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



