#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <string>
#ifdef IoTeyeDEBUG
#include <iostream>
#endif // !IoTeyeDEBUG

namespace func
{
    uint8_t GetCommandCode(const std::string& cmd);
}

namespace iotDebug
{
    // Debug functions
    template <typename T>
    void debugMessage(const T &&mes)
    {
#ifdef IoTeyeDEBUG
        std::cout << mes;
#endif // !IoTeyeDEBUG
    }

    template <typename T>
    void debugMessage(const T &mes)
    {
#ifdef IoTeyeDEBUG
        std::cout << mes;
#endif // !IoTeyeDEBUG
    }
#define NEWLINE debugMessage("\n");
}

#endif //FUNCTIONAL_H