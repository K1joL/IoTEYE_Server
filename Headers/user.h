#ifndef USERCLASS_H
#define USERCLASS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include "device.h"

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
            int changePin(uint8_t pinNumber, const std::string& data);
            int removePin(uint8_t pinNumber);
            std::string getPin(uint8_t pinNumber);
            std::string getID() { return m_userID; }
            static uint64_t getNextID();
            int addDevice(uint64_t devID, ioteyeDevice::Device* device);
            void pingDevice(uint64_t devID);
            //returns:
            //0 - if device is online
            //1 - if device is offline 
            //2 - if information is out of date
            uint8_t getDeviceStatus(uint64_t devID);
        private:
            std::string m_userID;
            size_t m_pinsCounter = 0;
            size_t m_maxPins = 255;
            static uint64_t m_idSequence;

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
        //Devices data
            std::unordered_map<uint64_t, ioteyeDevice::Device*> m_devices;
    };
}

#endif //USERCLASS_H