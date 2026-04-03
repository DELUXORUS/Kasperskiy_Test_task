#include <exception>
#include <iostream>

#include "analytics.hpp"


int main()
{
    try
    {
        Analytics analytics;
        analytics.run();
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}