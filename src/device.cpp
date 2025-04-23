/*# MIT License

# Copyright (c) 2025 Shults Bogdan aka K1joL

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

#include "device.h"

#include "functional.h"
namespace ioteye {
using namespace server::debug;

Device::StateTimer::StateTimer(std::function<void(uint8_t)> callback,
                               std::mutex& mutex)
    : m_changingMutex(mutex), m_cbChangeState{callback}, m_isStopped(false) {
    m_timerThread = std::thread([this]() { threadLoop(); });
}

void Device::StateTimer::threadLoop() {
    while (!m_isStopped) {
        if (m_wasPing) {
            m_start = chronoClock::now();
            m_wasPing = false;
            changeState(STATES::ONLINE);
        }

        auto now = chronoClock::now();
        auto elapsed = now - m_start;

        switch (m_currentState) {
            case STATES::ONLINE:
                if (std::chrono::duration_cast<ms>(elapsed) >=
                    m_outdatedDelay) {
                    changeState(OUTDATED);
                }
                break;
            case STATES::OUTDATED:
                if (std::chrono::duration_cast<ms>(elapsed) >= m_offlineDelay) {
                    changeState(OFFLINE);
                }
                break;
            case STATES::OFFLINE:
                // Do nothing while waiting for the ping
                break;
        }
    }
}

void Device::StateTimer::ping() {
    m_wasPing = true;
}

void Device::StateTimer::setPingFalse() {
    m_wasPing = false;
}

bool Device::StateTimer::wasPing() {
    return m_wasPing;
}

void Device::StateTimer::stop() {
    m_isStopped = true;
}

void Device::StateTimer::setDelays(ms outdatedDelay, ms offlineDelay) {
    std::lock_guard<std::mutex> lock(m_changingMutex);
    m_outdatedDelay = outdatedDelay;
    m_offlineDelay = offlineDelay;
}

ms Device::StateTimer::getRemainingTime() const {
    auto now = chronoClock::now();
    auto elapsed = now - m_start;
    auto remaining = m_offlineDelay - std::chrono::duration_cast<ms>(elapsed);
    return (remaining > ms(0)) ? remaining : ms(0);
}

bool Device::StateTimer::isStopped() const {
    return m_isStopped;
}

void Device::StateTimer::changeState(uint8_t newState) {
    m_currentState = newState;
    std::lock_guard<std::mutex> lock(m_changingMutex);
    m_cbChangeState(newState);
}

Device::StateTimer::~StateTimer() {
    // log("StateTimer desctructor");
    stop();
    if (m_timerThread.joinable())
        m_timerThread.join();
}

uint64_t Device::m_idSequence = 1;

Device::Device() {
    m_id = m_idSequence;
    m_idSequence++;
    m_stateTimer = std::make_shared<StateTimer>(
        std::bind(&Device::changeState, this, std::placeholders::_1),
        m_timerMutex);
    generateToken();
}

Device::Device(uint16_t outdatedDelay, uint16_t offlineDelay, uint16_t maxPins)
    : Device() {
    m_stateTimer->setDelays(ms(outdatedDelay), ms(offlineDelay));
    m_maxPins = maxPins;
}

Device::Device(Device&& other) noexcept
    : m_id(other.m_id),
      m_token(std::move(other.m_token)),
      m_pinsType(std::move(other.m_pinsType)),
      m_intPins(std::move(other.m_intPins)),
      m_doublePins(std::move(other.m_doublePins)),
      m_stringPins(std::move(other.m_stringPins)),
      m_pinsCounter(other.m_pinsCounter),
      m_maxPins(other.m_maxPins) {
    m_stateTimer = std::make_shared<StateTimer>(
        std::bind(&Device::changeState, this, std::placeholders::_1),
        m_timerMutex);
    m_idSequence = other.m_id + 1;
}

Device& Device::operator=(Device&& other) noexcept {
    if (this != &other) {
        m_id = other.m_id;
        m_token = std::move(other.m_token);
        m_pinsType = std::move(other.m_pinsType);
        m_intPins = std::move(other.m_intPins);
        m_doublePins = std::move(other.m_doublePins);
        m_stringPins = std::move(other.m_stringPins);
        m_pinsCounter = other.m_pinsCounter;
        m_maxPins = other.m_maxPins;
        m_stateTimer = std::make_shared<StateTimer>(
            std::bind(&Device::changeState, this, std::placeholders::_1),
            m_timerMutex);
        m_idSequence = other.m_id + 1;
    }
    return *this;
}

void Device::generateToken() {
    auto token =
        jwt::create()
            .set_type("JWS")
            .set_payload_claim("deviceID", jwt::claim(std::to_string(m_id)))
            .sign(jwt::algorithm::hs256{
                std::to_string(m_stateTimer->getRemainingTime().count()) +
                std::to_string(65 + m_idSequence % 26)});
    m_token = token;
}

std::string Device::getToken() {
    return m_token;
}

uint8_t Device::getState() {
    return m_state;
}

void Device::changeState(uint8_t state) {
    if (state != m_state) {
        m_state = state;
        // log("Changed State to ", char(state + 48));
    }
}

Device::~Device() {
    // log("device desctructor");
}

int Device::addPin(uint16_t pinNumber, const std::string& dataType,
                   const std::string& defaultData) {
    if (m_pinsCounter ==
        m_maxPins)  // the number of pins must be less than m_maxPins
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

int Device::changePin(uint16_t pinNumber, const std::string& data) {
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

int Device::removePin(uint16_t pinNumber) {
    if (m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;

    switch (m_pinsType.at(pinNumber)) {
        case INTID:
            m_intPins.erase(m_intPins.find(pinNumber),
                            ++m_intPins.find(pinNumber));
            break;
        case DOUBLEID:
            m_doublePins.erase(m_doublePins.find(pinNumber),
                               ++m_doublePins.find(pinNumber));
            break;
        case STRINGID:
            m_stringPins.erase(m_stringPins.find(pinNumber),
                               ++m_stringPins.find(pinNumber));
            break;
        default:
            // Invalid data type
            return 4;
    }
    m_pinsType.erase(m_pinsType.find(pinNumber), ++m_pinsType.find(pinNumber));
    m_pinsCounter--;

    return 0;
}

std::string Device::getPin(uint16_t pinNumber) {
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

uint16_t Device::pinsCreated() {
    return m_pinsCounter;
}
uint16_t Device::getMaxPins() {
    return m_maxPins;
}

// Getters for pins maps
const std::unordered_map<uint16_t, uint8_t>& Device::getPinsTypes() const {
    return m_pinsType;
}
const std::unordered_map<uint16_t, int>& Device::getIntPins() const {
    return m_intPins;
}

const std::unordered_map<uint16_t, double>& Device::getDoublePins() const {
    return m_doublePins;
}

const std::unordered_map<uint16_t, std::string>& Device::getStringPins() const {
    return m_stringPins;
}

// Device builder
Device::Builder::Builder()
    : m_outdatedDelay(500),
      m_offlineDelay(1000),
      m_maxPins(255),
      m_id(0),
      m_state(OFFLINE) {
}

Device::Builder& Device::Builder::setOutdatedDelay(uint16_t outdatedDelay) {
    m_outdatedDelay = outdatedDelay;
    return *this;
}

Device::Builder& Device::Builder::setOfflineDelay(uint16_t offlineDelay) {
    m_offlineDelay = offlineDelay;
    return *this;
}

Device::Builder& Device::Builder::setMaxPins(uint16_t maxPins) {
    m_maxPins = maxPins;
    return *this;
}

Device::Builder& Device::Builder::setID(uint64_t id) {
    m_id = id;
    return *this;
}

Device::Builder& Device::Builder::setToken(const std::string& token) {
    m_token = token;
    return *this;
}

Device::Builder& Device::Builder::setState(uint8_t state) {
    m_state = state;
    return *this;
}

Device::Builder& Device::Builder::setIntPin(uint16_t pinNumber, int value) {
    m_intPins[pinNumber] = value;
    setPinsTypePin(pinNumber, INTID);
    return *this;
}

Device::Builder& Device::Builder::setDoublePin(uint16_t pinNumber,
                                               double value) {
    m_doublePins[pinNumber] = value;
    setPinsTypePin(pinNumber, DOUBLEID);
    return *this;
}

Device::Builder& Device::Builder::setStringPin(uint16_t pinNumber,
                                               const std::string& value) {
    m_stringPins[pinNumber] = value;
    setPinsTypePin(pinNumber, STRINGID);
    return *this;
}

Device::Builder& Device::Builder::setPinsTypePin(uint16_t pinNumber,
                                                 uint8_t value) {
    m_pinsType[pinNumber] = value;
    return *this;
}

Device::Builder& Device::Builder::setIntPinMap(
    std::unordered_map<uint16_t, int>&& intPinMap) {
    m_intPins = intPinMap;
    return *this;
}

Device::Builder& Device::Builder::setDoublePinMap(
    std::unordered_map<uint16_t, double>&& doublePinMap) {
    m_doublePins = doublePinMap;
    return *this;
}

Device::Builder& Device::Builder::setStringPinMap(
    std::unordered_map<uint16_t, std::string>&& stringPinMap) {
    m_stringPins = stringPinMap;
    return *this;
}

Device::Builder& Device::Builder::setPinsTypeMap(
    std::unordered_map<uint16_t, uint8_t>&& pinsTypeMap) {
    m_pinsType = pinsTypeMap;
    return *this;
}

Device Device::Builder::build() {
    Device device(m_outdatedDelay, m_offlineDelay, m_maxPins);
    device.m_id = m_id;
    if (!m_token.empty())
        device.m_token = m_token;
    device.m_state = m_state;
    device.m_intPins = m_intPins;
    device.m_doublePins = m_doublePins;
    device.m_stringPins = m_stringPins;
    device.m_pinsType = m_pinsType;
    return device;
}

}  // namespace ioteye