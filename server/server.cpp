#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "server.hpp"
#include "nlohmann/json.hpp"


using json = nlohmann::json;


volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_childStop = 0;
int g_listenSocket = -1;


SharedStats* createSharedMemory(size_t patternCount, size_t& totalSize)
{
    totalSize = sizeof(SharedStats) + patternCount * sizeof(size_t);

    SharedStats* stats = static_cast<SharedStats*>(
        mmap(
            nullptr,
            totalSize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS,
            -1,
            0
        )
    );

    if (stats == MAP_FAILED)
    {
        throw std::runtime_error("[CreateSharedMemory] mmap() failed");
    }

    std::memset(stats, 0, totalSize);

    if (sem_init(&stats->semaphor, 1, 1) < 0)
    {
        munmap(stats, totalSize);
        throw std::runtime_error("[CreateSharedMemory] sem_init() failed");
    }

    return stats;
}

size_t* getPatternCounts(SharedStats* stats)
{
    return reinterpret_cast<size_t*>(stats + 1);
}

Server::Server(int port, const std::string& pathToCfg) : _port(port) 
{
    _setupSocket();
    _createSignals();
    _readPatterns(pathToCfg);

    _stats = createSharedMemory(_patterns.size(), _sharedStatsSize);
}

void Server::_updateStats(const std::unordered_map<size_t, size_t>& foundPatterns)
{
    sem_wait(&_stats->semaphor);

    _stats->countFiles++;

    size_t* countsPatterns = getPatternCounts(_stats);

    for (auto pair : foundPatterns)
    {
        countsPatterns[pair.first] += pair.second;
    }


    sem_post(&_stats->semaphor);
}

// void Server::_updateStats(const std::vector<int>& foundPatterns)
// {
//     sem_wait(&_stats->semaphor);

//     _stats->countFiles++;

//     size_t* countsPatterns = getPatternCounts(_stats);

//     for (int idx : foundPatterns)
//     {
//         if (idx >= 0 && static_cast<size_t>(idx) < _patterns.size())
//         {
//             countsPatterns[idx]++;
//         }
//     }

//     sem_post(&_stats->semaphor);
// }

Server::~Server()
{
    if (_stats != nullptr)
    {
        sem_destroy(&_stats->semaphor);
        munmap(_stats, _sharedStatsSize);
        _stats = nullptr;
        _sharedStatsSize = 0;
    }

    if (_socket >= 0)
    {
        close(_socket);
    }
}

void terminateHandler(int)
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

    std::cout << "[Server::_setupSocket] Listening on port " << _port << std::endl;
}

void Server::_createSignals()
{
    struct sigaction sa{};
    sa.sa_handler = terminateHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) < 0 ||
        sigaction(SIGTERM, &sa, nullptr) < 0)
    {
        close(_socket);
        throw std::runtime_error("sigaction error");
    }
}

void Server::_readPatterns(const std::string& pathToCfg)
{
    std::ifstream file(pathToCfg);

    if (!file.is_open())
    {
        std::cerr << "Failed to open config file: " << pathToCfg << std::endl;
        return;
    }

    json j;

    try
    {
        file >> j;

        if (!j.contains("patterns"))
        {
            std::cerr << "Config does not contain key \"patterns\"" << std::endl;
            return;
        }

        if (!j["patterns"].is_array())
        {
            std::cerr << "\"patterns\" must be an array" << std::endl;
            return;
        }

        _patterns = j["patterns"].get<std::vector<std::string>>();
    }
    catch (const json::exception& e)
    {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    std::cout << "[Server::_readPatterns] Malicious patterns have been read" << std::endl;
}

std::unordered_map<size_t, size_t> Server::_checkPatterns(const std::string& buffer)
{
    std::unordered_map<size_t, size_t> foundPatterns;

    for (size_t i = 0; i < _patterns.size(); ++i)
    {
        const std::string& pattern = _patterns[i];

        if (pattern.empty())
            continue;

        size_t pos = 0;
        while ((pos = buffer.find(pattern, pos)) != std::string::npos)
        {
            foundPatterns[i]++;
            pos += pattern.size();
        }
    }

    return foundPatterns;
}

// std::vector<size_t> Server::_checkPatterns(std::string& buffer)
// {
//     std::vector<size_t> foundIndexes;

//     for (size_t i = 0; i < _patterns.size(); ++i)
//     {
//         if (buffer.find(_patterns[i]) != std::string::npos)
//         {
//             foundIndexes.push_back(i);
//         }
//     }

//     return foundIndexes;
// }

void mergeMaps(std::unordered_map<size_t, size_t>& foundPatterns,
               std::unordered_map<size_t, size_t>& foundPatternsBuffer)
{
    for (auto pair : foundPatternsBuffer)
    {
        if (foundPatterns.find(pair.first) != foundPatterns.end())
        {
            foundPatterns[pair.first] += pair.second;
        }
        else
        {
            foundPatterns.insert(pair);
        }
    }
}

void Server::_handleClient(int clientSocket)
{   
    g_childStop = 0;

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
    std::unordered_map<size_t, size_t> foundPatterns;
    while (!g_childStop)
    {
        ssize_t numSymb = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (numSymb == 0)
        {
            std::cout << "[Server::_handleClient] child_PID: The server has received all the data from the client. Completing the connection" << std::endl;
            break;
        }
        else if (numSymb < 0)
        {
            if (errno == EINTR)
            continue;
            
            break;

        }

        std::string bufStr(buffer, numSymb);

        std::unordered_map<size_t, size_t> foundPatternsBuffer = _checkPatterns(bufStr);

        mergeMaps(foundPatterns, foundPatternsBuffer);

        // for (auto pair : foundPatternsBuffer)
        // {
        //     if (foundPatterns.find(pair.first) != foundPatterns.end())
        //     {
        //         foundPatterns[pair.first] += pair.second;
        //     }
        //     else
        //     {
        //         foundPatterns.insert(pair);
        //     }
        // }
        // _updateStats(_checkPatterns(bufStr));
    }

    _updateStats(foundPatterns);
    

    std::ostringstream response;
    response << "Detected threats count: " << foundPatterns.size() << "\n";

    if (foundPatterns.empty())
    {
        response << "No malicious patterns detected\n";
    }
    else
    {
        response << "Threat types:\n";

        for (const auto& pair : foundPatterns)
        {
            size_t patternIndex = pair.first;
            size_t threatCount = pair.second;

            if (patternIndex < _patterns.size())
            {
                response << _patterns[patternIndex]
                         << " -> "
                         << threatCount
                         << "\n";
            }
        }
    }

    std::string msg = response.str();

    size_t totalSent = 0;
    while (totalSent < msg.size())
    {
        ssize_t sent = send(clientSocket,
                            msg.data() + totalSent,
                            msg.size() - totalSent,
                            0);

        if (sent < 0)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "[Server::_handleClient] send() error" << std::endl;
            break;
        }

        totalSent += sent;
    }
    // if (numSymb > 0)
    // {
        // std::cout << "[Server::_handleClient] child_PID: " << getpid() << " the file has been checked for threats" << std::endl;
    // }

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
    if (_statsPid > 0)
    {
        kill(_statsPid, SIGTERM);
        waitpid(_statsPid, nullptr, 0);
    }

    unlink("/tmp/stats_fifo_req");
    unlink("/tmp/stats_fifo_resp");

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

std::string Server::_getStats()
{
    sem_wait(&_stats->semaphor);

    std::ostringstream response;
    response << "Processed files: " << _stats->countFiles << "\n";

    size_t* counts = getPatternCounts(_stats);

    for (size_t i = 0; i < _patterns.size(); ++i)
    {
        response << _patterns[i] << " -> " << counts[i] << "\n";
    }

    sem_post(&_stats->semaphor);

    std::string msg = response.str(); 
    return msg;
}

void Server::_runStatsIpc()
{
    if (mkfifo(_fifoReq, 0666) < 0 && errno != EEXIST)
    {
        throw std::runtime_error("[Server::_runStatsIpc] mkfifo request failed");
    }

    if (mkfifo(_fifoResp, 0666) < 0 && errno != EEXIST)
    {
        throw std::runtime_error("[Server::_runStatsIpc] mkfifo response failed");
    }

    while (g_running)
    {
        int reqFd = open(_fifoReq, O_RDONLY);
        if (reqFd < 0)
        {
            continue;
        }

        char buffer[256];
        ssize_t n = read(reqFd, buffer, sizeof(buffer));
        close(reqFd);

        if (n <= 0)
        {
            continue;
        }

        std::string request(buffer, n);

        if (request.find("GET_STATS") == std::string::npos)
        {
            continue;
        }

        std::string msg = _getStats();

        int respFd = open(_fifoResp, O_WRONLY);
        if (respFd >= 0)
        {
            write(respFd, msg.c_str(), msg.size());
            close(respFd);
        }
    }
}

void Server::run()
{
    _statsPid= fork();

    if (_statsPid == 0)
    {
        _runStatsIpc();
        _exit(0);
    }

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

        std::cout << std::endl << "[Server::run] New connection from "
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



