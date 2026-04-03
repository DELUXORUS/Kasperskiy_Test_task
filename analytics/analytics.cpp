#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <exception>
#include <string>

#include "analytics.hpp"



void Analytics::_sendRequest()
{
    const char* FIFO_REQ = "/tmp/stats_fifo_req";
    std::string request = "GET_STATS\n";

    int fd = open(FIFO_REQ, O_WRONLY);

    if (fd < 0)
    {
        throw std::runtime_error("[Analytics::_sendRequest] Open request file failed");
    }

    if(write(fd, request.c_str(), request.size()) < 0)
    {
        close(fd);
        throw std::runtime_error("[Analytics::_sendRequest] Write failed");
    }

    close(fd);
}

std::string Analytics::_readResponse()
{
    const char* FIFO_RESP = "/tmp/stats_fifo_resp";

    int fd = open(FIFO_RESP, O_RDONLY);
    if (fd < 0)
    {
        throw std::runtime_error("[Analytics::_sendRequest] Open response file failed");
    }

    std::string response;
    char buffer[512];

    while (true)
    {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
        if (bytesRead < 0)
        {
            close(fd);
            throw std::runtime_error("[Analytics::_readResponse] Read failed");
        }

        if (bytesRead == 0)
        {
            break;
        }

        response.append(buffer, bytesRead);
    }

    close(fd);
    return response;
}

void Analytics::_displayStats()
{
    std::cout << "::Server statistics on verified files at the current moment::" << std::endl;
    std::cout << _stats << std::endl;
}

void Analytics::run()
{
    _sendRequest();
    _stats = _readResponse();
    _displayStats();
}
