#ifndef USERCLASS_H
#define USERCLASS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

using std::cout;
using std::endl;

namespace ioteyeUser
{
    class User
    {
        public:
            User(const std::string& userID)
            {
                this->m_userID = userID;
            }
            int addPin(uint8_t pinNumber, const std::string& dataType, const std::string& defaultData);
            template<typename T>
            int changePin(uint8_t pinNumber, const T& data);
            int removePin(uint8_t pinNumber);

        private:
            std::string m_userID;
            size_t m_pinsCounter = 0;

        enum ContainerID
        {
            INTID = 105,
            DOUBLEID = 100,
            STRINGID = 115
        };
        // Virtual pins data
            std::unordered_map<uint8_t, uint8_t> m_pinsType;
            std::unordered_map<uint8_t, int> m_intPins;
            std::unordered_map<uint8_t, double> m_doublePins;
            std::unordered_map<uint8_t, std::string> m_stringPins;
    };
}

#endif //USERCLASS_H