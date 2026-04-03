#ifndef ANALYTICS_H_
#define ANALYTICS_H_


#include <string>


class Analytics
{
    public:
        Analytics() {};

        void run();
    private:
        std::string _stats;

        void _sendRequest();
        std::string _readResponse();
        void _displayStats();
};


#endif