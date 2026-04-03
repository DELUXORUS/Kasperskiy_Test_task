#ifndef SERVER_HPP_
#define SERVER_HPP_


#include <sys/mman.h>
#include <semaphore.h>
#include <string>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <vector>


struct SharedStats
{
    sem_t semaphor;
    size_t countFiles;
};


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
        int _statsPid;
        std::vector<std::string> _patterns; 

        SharedStats* _stats;
        size_t _sharedStatsSize;
        
        const char* _fifoReq  = "/tmp/stats_fifo_req";
        const char* _fifoResp = "/tmp/stats_fifo_resp";

        void _setupSocket();
        void _createSignals();
        void _readPatterns(const std::string& pathToCfg);
        void _handleClient(int clientSocket);
        void _removeZombie();
        void _shutdown(); 
        void _updateStats(const std::unordered_map<size_t, size_t>& foundPatterns);
        std::unordered_map<size_t, size_t> _checkPatterns(const std::string& buffer);
        std::string _getStats(); 
        void _runStatsIpc();
        // void _updateStats(const std::vector<int>& foundPatterns);
        // std::vector<size_t> _checkPatterns(std::string& buffer);
};  


#endif