#include "user.h" 
#ifdef IOTEYE_USER_DEBUG
#include "functional.h"
using namespace iotDebug;
#endif // !IOTEYE_USER_DEBUG


uint64_t ioteyeUser::User::m_idSequence = 0;

int ioteyeUser::User::addPin(uint8_t pinNumber, const std::string& dataType, 
    const std::string& defaultData)
{
    if(m_pinsCounter == 255) // the number of pins must be less than 255
    {
#ifdef IOTEYE_USER_DEBUG
        debugMessage("the number of pins must be less than 255");
#endif // !IOTEYE_USER_DEBUG
        return 2;
    }
    if(dataType.empty()) // if data type not specified
    {
#ifdef IOTEYE_USER_DEBUG
        debugMessage("data type is not specified");
#endif // !IOTEYE_USER_DEBUG
        return 1;
    }
    if(m_pinsType.find(pinNumber) != m_pinsType.end()) // is pin exists
    {
#ifdef IOTEYE_USER_DEBUG
        debugMessage("pin exists");
#endif // !IOTEYE_USER_DEBUG
        return 3;
    }

    switch(std::tolower(dataType[0]))
    {
    case INTID:
        m_intPins.emplace(pinNumber, std::stoi(defaultData));
        break;
    case DOUBLEID:
        m_doublePins.emplace(pinNumber, std::stod(defaultData));
        break;
    case STRINGID:
        m_stringPins.emplace(pinNumber, defaultData);
        break;
    default:
        // Invalid data type
        return 4;
    }
    m_pinsType.emplace(pinNumber, std::tolower(dataType[0]));
    m_pinsCounter++;
    
    // OK
    return 0;
}

int ioteyeUser::User::changePin(uint8_t pinNumber, const std::string& data)
{
    if(m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;
    
    switch(m_pinsType.at(pinNumber))
    {
    case INTID:
        m_intPins.at(pinNumber) = std::stoi(data);
        break;
    case DOUBLEID:
        m_doublePins.at(pinNumber) = std::stod(data);
        break;
    case STRINGID:
        m_stringPins.at(pinNumber) = data;
        break;
    default:
        // Invalid data type
        return 4;
    }

    return 0;
}

int ioteyeUser::User::removePin(uint8_t pinNumber)
{
    if(m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;
    
    switch(m_pinsType.at(pinNumber))
    {
    case INTID:
        m_intPins.erase(m_intPins.find(pinNumber), ++m_intPins.find(pinNumber));
        break;
    case DOUBLEID:
        m_doublePins.erase(m_doublePins.find(pinNumber), ++m_doublePins.find(pinNumber));
        break;
    case STRINGID:
        m_stringPins.erase(m_stringPins.find(pinNumber), ++m_stringPins.find(pinNumber));
        break;
    default:
        // Invalid data type
        return 4;
    }
    m_pinsType.erase(m_pinsType.find(pinNumber), ++m_pinsType.find(pinNumber));
    m_pinsCounter--;
    
    return 0;
}

std::string ioteyeUser::User::getPin(uint8_t pinNumber)
{
    if(m_pinsType.find(pinNumber) == m_pinsType.end())
        return std::string{""};
    
    switch(m_pinsType.at(pinNumber))
    {
    case INTID:
        return std::to_string(m_intPins.at(pinNumber));
    case DOUBLEID:
        return std::to_string(m_doublePins.at(pinNumber));
    case STRINGID:
        return m_stringPins.at(pinNumber);
    default:
        // Invalid data type
        return std::string{""};
    }
}

uint64_t ioteyeUser::User::getNextID()
{
    if(m_idSequence < UINT64_MAX)
        return m_idSequence++;
    else return 0;
}

int ioteyeUser::User::addDevice(uint64_t devID, ioteyeDevice::Device *device)
{
    m_devices.emplace(devID, device);
    return 0;
}

void ioteyeUser::User::pingDevice(uint64_t devID)
{
    auto device = m_devices.find(devID);
    if(device != m_devices.end())
        return device->second->ping();
}

uint8_t ioteyeUser::User::getDeviceStatus(uint64_t devID)
{
    auto device = m_devices.find(devID);
    if(device != m_devices.end())
        return device->second->getState();
    return ioteyeDevice::Device::MAX_STATES;
}
