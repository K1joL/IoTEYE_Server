#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <string>
#ifdef IoTeyeDEBUG
#include <iostream>
#include <httpserver.hpp>
#endif // !IoTeyeDEBUG

namespace func
{
    uint8_t GetCommandCode(const std::string &cmd);
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

template <typename T>
    void debugMessageln(const T &&mes)
    {
#ifdef IoTeyeDEBUG
        std::cout << mes;
        NEWLINE
#endif // !IoTeyeDEBUG
    }

    template <typename T>
    void debugMessageln(const T &mes)
    {
#ifdef IoTeyeDEBUG
        std::cout << mes;
        NEWLINE
#endif // !IoTeyeDEBUG
    }

#define PAYLOAD_DEBUG(req_args)                     \
    for (auto &e : req_args)                        \
    {                                               \
        debugMessage("Payload: ");                  \
        debugMessage(e.first);                      \
        debugMessage(": " + std::string(e.second)); \
        NEWLINE                                     \
    }

}

#endif // FUNCTIONAL_H