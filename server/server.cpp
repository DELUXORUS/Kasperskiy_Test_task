#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <signal.h>
#include <atomic>

#include "server.hpp"


volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_childStop = 0;
int g_listenSocket = -1;


Server::~Server()
{
    if (_socket >= 0)
    {
        close(_socket);
    }
}

void sigintHandler(int)
{
    g_running = 0;

    if (g_listenSocket >= 0)
    {
        close(g_listenSocket);
    }
}

//  for childs

void sigtermHandler(int)
{
    g_childStop = 1;
}

void Server::_setupSocket()
{
    _socket = socket(AF_INET, SOCK_STREAM, 0);
    g_listenSocket = _socket;

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

    struct sigaction saSigint{};
    saSigint.sa_handler = sigintHandler;
    sigemptyset(&saSigint.sa_mask);
    saSigint.sa_flags = 0;

    if (sigaction(SIGINT, &saSigint, nullptr) < 0)
    {
        close(_socket);
        throw std::runtime_error("Sigaction error");
    }

    std::cout << "[Server::_setupSocket] Listening on port " << _port << std::endl;
}

void Server::_handleClient(int clientSocket)
{
    g_childStop = false;

    struct sigaction saSigterm{};
    saSigterm.sa_handler = sigtermHandler;
    sigemptyset(&saSigterm.sa_mask);
    saSigterm.sa_flags = 0;

    if (sigaction(SIGTERM, &saSigterm, nullptr) < 0)
    {
        close(clientSocket);
        return;
    }

    char buffer[4096];

    while (!g_childStop)
    {
        int numSymb = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (numSymb == 0)
        {
            break;
        }

        if (numSymb < 0)
        {
            if (errno == EINTR)
                continue;

            break;
        }
    }

    close(clientSocket);
}

void Server::_removeZombie()
{
    while (true)
    {
        pid_t pid = waitpid(-1, nullptr, WNOHANG);

        if (pid <= 0)
        {
            break;
        }

        std::cout << "[Server::_removeZombie] remove zombie pid: " << pid << std::endl; 
        _childs.erase(pid);
    }
}

void Server::_shutdown()
{
    for (pid_t child : _childs)
    {
        std::cout << "[Server::_shutdown] kill pid: " << child << std::endl;
        if (kill(child, SIGTERM) < 0 && errno != ESRCH)
        {
            std::cerr << "[Server::run] kill() error for pid " << child << std::endl;
        }
    }

    while (!_childs.empty())
    {
        pid_t pid = waitpid(-1, nullptr, 0);

        if (pid > 0)
        {
            _childs.erase(pid);
        }
        else
        {
            if (errno == EINTR)
            {
                continue;
            }

            break;
        }
    }
}

void Server::run()
{
    while (g_running)
    {
        _removeZombie();
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSocket = accept(
            _socket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrLen
        );

        if (clientSocket < 0)
        {
            if (!g_running)
                break;

            if (errno == EINTR)
                continue;

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
            _childs.insert(pid);
            close(clientSocket);
        }
    }
    
    close(_socket);

    _removeZombie();
    _shutdown();
}



