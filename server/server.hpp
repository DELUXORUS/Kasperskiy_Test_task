#ifndef SERVER_HPP_
#define SERVER_HPP_


// #include <sys/mman.h>
// #include <semaphore.h>
// #include <unistd.h>
#include <string>
#include <unordered_set>
#include <vector>


// struct SharedStats
// {
//     sem_t sem;
//     unsigned int countFiles;
//     unsigned int countPatterns[];
// };


class Server
{
    public:
        Server(int port, const std::string& pathToCfg);

        void run();

        ~Server();
    private:
        int _socket;
        int _port;
        std::unordered_set<pid_t> _childs;
        std::vector<std::string> _patterns; 
        // SharedStats* _stats = nullptr;

        void _setupSocket();
        void _handleClient(int clientSocket);
        void _removeZombie();
        void _shutdown(); 
        void _readPatterns(const std::string& pathToCfg);
        void _createSignals();
        // int _checkPatterns(std::string potentialPatterns);
};  


#endif