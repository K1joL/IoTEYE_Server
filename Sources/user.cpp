#include "user.h" 

int ioteyeUser::User::addPin(uint8_t pinNumber, const std::string& dataType, 
    const std::string& defaultData)
{
    if(m_pinsCounter == 255) // the number of pins must be less than 255
    {
        cout << 2 << endl;
        return 2;
    }
    if(dataType.empty()) // if data type not specified
    {
        cout << 1 << endl;
        return 1;
    }
    if(m_pinsType.find(pinNumber) != m_pinsType.end()) // is pin exists
    {
        cout << 3 << endl;
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
