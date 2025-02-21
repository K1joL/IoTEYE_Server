#include "device.h"

#include "functional.h"
using namespace ioteye::server::debug;

ioteye::Device::StateTimer::StateTimer(std::function<void(uint8_t)> callback, std::mutex* mutex)
    : m_changingMutex(mutex), m_cbChangeState{callback} {
    std::thread([this]() { threadLoop(); }).detach();
}

// TODO: Rewrite loop fucntion with Finite-state machine
void ioteye::Device::StateTimer::threadLoop() {
    threadStarted();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (wasPing() || isStopped()) {
            setPingFalse();
            m_start = std::chrono::high_resolution_clock::now();
            while (std::chrono::high_resolution_clock::now() - m_start < m_outdatedDelay) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (wasPing() || isStopped())
                    break;
            }
            if (wasPing()) {
                m_cbChangeState(ioteye::Device::ONLINE);
                setPingFalse();
                continue;
            }
            // set state = outdated
            m_cbChangeState(ioteye::Device::OUTDATED);
            m_start = std::chrono::high_resolution_clock::now();
            while (std::chrono::high_resolution_clock::now() - m_start < m_offlineDelay) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (wasPing() || isStopped())
                    break;
            }
            if (wasPing()) {
                m_cbChangeState(ioteye::Device::ONLINE);
                setPingFalse();
                continue;
            }
            // set state = offline
            m_cbChangeState(ioteye::Device::OFFLINE);
        }
        if (isStopped())
            break;
    }
    killThread();
}

void ioteye::Device::StateTimer::ping() {
    m_changingMutex->lock();
    m_wasPing = true;
    m_changingMutex->unlock();
}

void ioteye::Device::StateTimer::setPingFalse() {
    m_changingMutex->lock();
    m_wasPing = false;
    m_changingMutex->unlock();
}

bool ioteye::Device::StateTimer::wasPing() {
    bool temp = false;
    m_changingMutex->lock();
    temp = m_wasPing;
    m_changingMutex->unlock();
    return temp;
}

void ioteye::Device::StateTimer::stop() {
    m_changingMutex->lock();
    m_isStopped = true;
    m_changingMutex->unlock();
}

void ioteye::Device::StateTimer::setDelays(ms outdatedDelay, ms offlineDelay) {
    m_changingMutex->lock();
    m_outdatedDelay = outdatedDelay;
    m_offlineDelay = offlineDelay;
    m_changingMutex->unlock();
}

ms ioteye::Device::StateTimer::getRemainingTime() {
    ms temp(0);
    m_changingMutex->lock();
    temp = ms((std::chrono::high_resolution_clock::now() - m_start).count());
    m_changingMutex->unlock();
    return temp;
}

void ioteye::Device::StateTimer::threadStarted() {
    m_changingMutex->lock();
    m_threadAlive = true;
    m_changingMutex->unlock();
}

void ioteye::Device::StateTimer::killThread() {
    m_changingMutex->lock();
    m_threadAlive = false;
    m_changingMutex->unlock();
}

bool ioteye::Device::StateTimer::isThreadAlive() {
    bool temp = false;
    m_changingMutex->lock();
    temp = m_threadAlive;
    m_changingMutex->unlock();
    return temp;
}

bool ioteye::Device::StateTimer::isStopped() {
    bool temp = false;
    m_changingMutex->lock();
    temp = m_isStopped;
    m_changingMutex->unlock();
    return temp;
}

uint64_t ioteye::Device::m_idSequence = 1;

ioteye::Device::Device() {
    m_id = m_idSequence;
    m_idSequence++;
    m_timerMutex = new std::mutex();
    m_stateTimer = new StateTimer{std::bind(&Device::changeState, this, std::placeholders::_1), m_timerMutex};
    generateToken();
}

ioteye::Device::Device(uint16_t outdatedDelay, uint16_t offlineDelay, uint16_t maxPins) : Device() {
    m_stateTimer->setDelays(ms(outdatedDelay), ms(offlineDelay));
    m_maxPins = maxPins;
}

void ioteye::Device::generateToken() {
    auto token = jwt::create()
                     .set_type("JWS")
                     .set_payload_claim("deviceID", jwt::claim(std::to_string(m_id)))
                     .sign(jwt::algorithm::hs256{std::to_string(m_stateTimer->getRemainingTime().count()) +
                                                 std::to_string(65 + m_idSequence % 26)});
    m_token = token;
}

std::string ioteye::Device::getToken() {
    return m_token;
}

uint8_t ioteye::Device::getState() {
    m_timerMutex->lock();
    uint8_t temp = m_state;
    m_timerMutex->unlock();
    return temp;
}

void ioteye::Device::changeState(uint8_t state) {
    m_timerMutex->lock();
    m_state = state;
    m_timerMutex->unlock();
}

ioteye::Device::~Device() {
    m_stateTimer->stop();
    while (m_stateTimer->isThreadAlive())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    delete m_timerMutex;
    delete m_stateTimer;
}

int ioteye::Device::addPin(uint16_t pinNumber, const std::string& dataType, const std::string& defaultData) {
    if (m_pinsCounter == m_maxPins)  // the number of pins must be less than m_maxPins
    {
        log("the number of pins must be less than ", m_maxPins);
        return 2;
    }
    if (dataType.empty())  // if data type not specified
    {
        log("data type is not specified");
        return 1;
    }
    if (m_pinsType.find(pinNumber) != m_pinsType.end())  // is pin exists
    {
        log("pin exists");
        return 3;
    }

    switch (std::tolower(dataType[0])) {
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

int ioteye::Device::changePin(uint16_t pinNumber, const std::string& data) {
    if (m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;

    switch (m_pinsType.at(pinNumber)) {
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

int ioteye::Device::removePin(uint16_t pinNumber) {
    if (m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;

    switch (m_pinsType.at(pinNumber)) {
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

std::string ioteye::Device::getPin(uint16_t pinNumber) {
    if (m_pinsType.find(pinNumber) == m_pinsType.end())
        return std::string{""};

    switch (m_pinsType.at(pinNumber)) {
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
