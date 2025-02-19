#include "device.h"

#ifdef IOTEYE_DEVICE_DEBUG
#include "functional.h"
using namespace iotDebug;
#endif // !IOTEYE_DEVICE_DEBUG

ioteyeDevice::Device::StateTimer::StateTimer(std::function<void(uint8_t)> callback, std::mutex* mutex)
: 
m_changingMutex(mutex),
m_cbChangeState{callback}
{
    std::thread([this]()
                { threadLoop(); })
        .detach();
}

// TODO: Rewrite loop fucntion with Finite-state machine
void ioteyeDevice::Device::StateTimer::threadLoop()
{   
    threadStarted();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (wasPing() || isStopped())
        {
            setPingFalse();
            m_start = std::chrono::high_resolution_clock::now();
            while (std::chrono::high_resolution_clock::now() - m_start < m_outdatedDelay)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (wasPing() || isStopped())
                    break;
            }
            if (wasPing())
            {
                m_cbChangeState(ioteyeDevice::Device::ONLINE);
                setPingFalse();
                continue;
            }
            // set state = outdated
            m_cbChangeState(ioteyeDevice::Device::OUTDATED);
            m_start = std::chrono::high_resolution_clock::now();
            while (std::chrono::high_resolution_clock::now() - m_start < m_offlineDelay)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (wasPing() || isStopped())
                    break;
            }
            if (wasPing())
            {
                m_cbChangeState(ioteyeDevice::Device::ONLINE);
                setPingFalse();
                continue;
            }
            // set state = offline
            m_cbChangeState(ioteyeDevice::Device::OFFLINE);
        }
        if (isStopped())
            break;
    }
    killThread();
}

void ioteyeDevice::Device::StateTimer::ping()
{
    m_changingMutex->lock();
    m_wasPing = true;
    m_changingMutex->unlock();

}

void ioteyeDevice::Device::StateTimer::setPingFalse()
{
    m_changingMutex->lock();
    m_wasPing = false;
    m_changingMutex->unlock();
}

bool ioteyeDevice::Device::StateTimer::wasPing()
{
    bool temp = false;
    m_changingMutex->lock();
    temp = m_wasPing;
    m_changingMutex->unlock();
    return temp;
}

void ioteyeDevice::Device::StateTimer::stop()
{
    m_changingMutex->lock();
    m_isStopped = true;
    m_changingMutex->unlock();
}

void ioteyeDevice::Device::StateTimer::setDelays(ms outdatedDelay, ms offlineDelay)
{
    m_changingMutex->lock();
    m_outdatedDelay = outdatedDelay;
    m_offlineDelay = offlineDelay;
    m_changingMutex->unlock();
}

ms ioteyeDevice::Device::StateTimer::getRemainingTime()
{
    ms temp(0);
    m_changingMutex->lock();
    temp = ms((std::chrono::high_resolution_clock::now() - m_start).count());
    m_changingMutex->unlock();
    return temp;
}

void ioteyeDevice::Device::StateTimer::threadStarted()
{
    m_changingMutex->lock();
    m_threadAlive = true;
    m_changingMutex->unlock();
}

void ioteyeDevice::Device::StateTimer::killThread()
{
    m_changingMutex->lock();
    m_threadAlive = false;
    m_changingMutex->unlock();
}

bool ioteyeDevice::Device::StateTimer::isThreadAlive()
{
    bool temp = false;
    m_changingMutex->lock();
    temp = m_threadAlive;
    m_changingMutex->unlock();
    return temp;
}

bool ioteyeDevice::Device::StateTimer::isStopped()
{
    bool temp = false;
    m_changingMutex->lock();
    temp = m_isStopped;
    m_changingMutex->unlock();
    return temp;
}

uint64_t ioteyeDevice::Device::m_idSequence = 1;

ioteyeDevice::Device::Device()
{
    m_id = m_idSequence;
    m_idSequence++;
    m_timerMutex = new std::mutex();
    m_stateTimer = new StateTimer{std::bind(&Device::changeState, this, std::placeholders::_1), m_timerMutex};
    generateToken();
}

ioteyeDevice::Device::Device(uint16_t outdatedDelay, uint16_t offlineDelay, uint16_t maxPins) : Device()
{
    m_stateTimer->setDelays(ms(outdatedDelay), ms(offlineDelay));
    m_maxPins = maxPins;
}

void ioteyeDevice::Device::generateToken()
{
    
    auto token = jwt::create()
                     .set_type("JWS")
                     .set_payload_claim("deviceID", jwt::claim(std::to_string(m_id)))
                     .sign(jwt::algorithm::hs256{std::to_string(m_stateTimer->getRemainingTime().count()) + std::to_string(65 + m_idSequence % 26)});
    m_token = token;
}

std::string ioteyeDevice::Device::getToken()
{
    return m_token;
}

uint8_t ioteyeDevice::Device::getState()
{
    m_timerMutex->lock();
    uint8_t temp = m_state;
    m_timerMutex->unlock();
    return temp;
}

void ioteyeDevice::Device::changeState(uint8_t state)
{
    m_timerMutex->lock();
    m_state = state;
    m_timerMutex->unlock();
}

ioteyeDevice::Device::~Device()
{
    m_stateTimer->stop();
    while(m_stateTimer->isThreadAlive()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    delete m_timerMutex;
    delete m_stateTimer;
}

int ioteyeDevice::Device::addPin(uint16_t pinNumber, const std::string& dataType, 
    const std::string& defaultData)
{
    if(m_pinsCounter == m_maxPins) // the number of pins must be less than m_maxPins
    {
#ifdef IOTEYE_DEVICE_DEBUG
        debugMessage("the number of pins must be less than ");
        debugMessage(m_maxPins); NEWLINE
#endif // !IOTEYE_DEVICE_DEBUG
        return 2;
    }
    if(dataType.empty()) // if data type not specified
    {
#ifdef IOTEYE_DEVICE_DEBUG
        debugMessage("data type is not specified"); NEWLINE
#endif // !IOTEYE_DEVICE_DEBUG
        return 1;
    }
    if(m_pinsType.find(pinNumber) != m_pinsType.end()) // is pin exists
    {
#ifdef IOTEYE_DEVICE_DEBUG
        debugMessage("pin exists"); NEWLINE
#endif // !IOTEYE_DEVICE_DEBUG
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

int ioteyeDevice::Device::changePin(uint16_t pinNumber, const std::string& data)
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

int ioteyeDevice::Device::removePin(uint16_t pinNumber)
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

std::string ioteyeDevice::Device::getPin(uint16_t pinNumber)
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
