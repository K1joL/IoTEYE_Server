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

template<typename T>
int ioteyeUser::User::changePin(uint8_t pinNumber, const T& data)
{
    if(m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;
    
    switch(m_pinsType.at(pinNumber))
    {
    case INTID:
        m_intPins.at(pinNumber) = static_cast<int>(data);
        break;
    case DOUBLEID:
        m_doublePins.at(pinNumber) = static_cast<double>(data);
        break;
    case STRINGID:
        m_stringPins.at(pinNumber) = static_cast<std::string>(data);
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
    m_pinsCounter--;
    
    return 0;
}