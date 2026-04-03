#include <iostream>
#include <exception>

#include "client.hpp"


int main(int argc, char** argv)
{
    if (argc != 3)
    {
        throw std::invalid_argument("Please, enter path to file and port of server!");
    }

    std::string pathToFile = argv[1];
    int port = atoi(argv[2]);

    Client client;
    client.sendFile(pathToFile, port);
    
}