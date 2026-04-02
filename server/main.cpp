#include <iostream>
#include <exception>

#include "server.hpp"


int main(int argc, char** argv)
{
    if (argc != 3)
    {
        throw std::invalid_argument("Please, enter path to file and port of server!");
    }

    std::string pathToCfg = argv[1];
    int port = atoi(argv[2]);

    Server server(port, pathToCfg);
    server.run();
    
}