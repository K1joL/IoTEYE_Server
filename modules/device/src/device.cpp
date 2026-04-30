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

#include <common/logging.hpp>
#include <device/device.hpp>

namespace ioteye {
using namespace ioteye::types;
using namespace server::debug;

Device::StateTimer::StateTimer(std::function<void(DeviceState)> callback,
                               std::mutex& mutex)
    : m_changingMutex(mutex), m_cbChangeState{callback}, m_isStopped(false) {
    m_timerThread = std::thread([this]() { threadLoop(); });
}

Device::StateTimer::StateTimer(std::function<void(DeviceState)> callback,
                               std::mutex& mutex, ms offlineDelay,
                               ms outdatedDelay)
    : StateTimer(callback, mutex) {
    m_offlineDelay = offlineDelay;
    m_outdatedDelay = outdatedDelay;
}

void Device::StateTimer::threadLoop() {
    while (!m_isStopped) {
        if (m_wasPing) {
            m_start = chronoClock::now();
            m_wasPing = false;
            changeState(DeviceState::ONLINE);
        }

        auto now = chronoClock::now();
        auto elapsed = now - m_start;

        switch (m_currentState) {
            case DeviceState::ONLINE:
                if (std::chrono::duration_cast<ms>(elapsed) >=
                    m_outdatedDelay) {
                    changeState(DeviceState::OUTDATED);
                }
                break;
            case DeviceState::OUTDATED:
                if (std::chrono::duration_cast<ms>(elapsed) >= m_offlineDelay) {
                    changeState(DeviceState::OFFLINE);
                }
                break;
            case DeviceState::OFFLINE:
                if (std::chrono::duration_cast<ms>(elapsed) >= m_deadDelay) {
                    changeState(DeviceState::DEAD);
                }
                break;
            case DeviceState::DEAD:
                // Wait until destructed
                break;
            default:
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

void Device::StateTimer::setDelays(ms outdatedDelay, ms offlineDelay,
                                   ms deadDelay) {
    std::lock_guard<std::mutex> lock(m_changingMutex);
    m_outdatedDelay = outdatedDelay;
    m_offlineDelay = offlineDelay;
    m_deadDelay = deadDelay;
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

void Device::StateTimer::changeState(DeviceState newState) {
    m_currentState = newState;
    std::lock_guard<std::mutex> lock(m_changingMutex);
    m_cbChangeState(newState);
}

ms Device::StateTimer::getOfflineDelay() {
    return m_offlineDelay;
}

ms Device::StateTimer::getOutdatedDelay() {
    return m_outdatedDelay;
}

Device::StateTimer::~StateTimer() {
    // log("StateTimer desctructor");
    stop();
    if (m_timerThread.joinable())
        m_timerThread.join();
}

DeviceID Device::m_idSequence = 1;

Device::Device() {
    m_id = m_idSequence;
    m_idSequence++;
    m_stateTimer = std::make_shared<StateTimer>(
        std::bind(&Device::changeState, this, std::placeholders::_1),
        m_timerMutex);
    generateToken();
}

Device::Device(delayMs outdatedDelay, delayMs offlineDelay, delayMs deadDelay,
               pinsQuantity maxPins, bool /* deleteAfterOffline */)
    : Device() {
    m_stateTimer->setDelays(ms(outdatedDelay), ms(offlineDelay), ms(deadDelay));
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
        m_timerMutex, other.m_stateTimer->getOfflineDelay(),
        other.m_stateTimer->getOutdatedDelay());
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
            m_timerMutex, other.m_stateTimer->getOfflineDelay(),
            other.m_stateTimer->getOutdatedDelay());
        adjustIdSequence(other.m_id);
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

DeviceID Device::getID() {
    return m_id;
}

DeviceState Device::getState() const {
    return m_state;
}

void Device::changeState(DeviceState state) {
    if (state != m_state) {
        m_state = state;
        // log("Changed State to ", char(state + 48));
    }
    if (m_onStateChange)
        m_onStateChange(state);
}

void Device::ping() {
    m_stateTimer->ping();
}

Device::~Device() {
    // log("device desctructor");
}

void Device::setOnStateChange(std::function<void(DeviceState)> cb) {
    m_onStateChange = std::move(cb);
}

int Device::addPin(pinId pinNumber, const std::string& dataType,
                   const std::string& defaultData) {
    // the number of pins must be less than m_maxPins
    if (m_pinsCounter == m_maxPins) {
        log(LogLevel::WARNING, "the number of pins must be less than ",
            m_maxPins);
        return 2;
    }
    if (dataType.empty())  // if data type not specified
    {
        log(LogLevel::WARNING, "data type is not specified");
        return 1;
    }
    if (m_pinsType.find(pinNumber) != m_pinsType.end())  // is pin exists
    {
        logStatus("pin exists");
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
    m_pinsType.emplace(pinNumber,
                       static_cast<ContainerID>(std::tolower(dataType[0])));
    m_pinsCounter++;

    // OK
    return 0;
}

int Device::changePin(pinId pinNumber, const std::string& data) {
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

int Device::removePin(pinId pinNumber) {
    if (m_pinsType.find(pinNumber) == m_pinsType.end())
        return 1;

    switch (m_pinsType.at(pinNumber)) {
        case INTID:
            m_intPins.erase(pinNumber);
            break;
        case DOUBLEID:
            m_doublePins.erase(pinNumber);
            break;
        case STRINGID:
            m_stringPins.erase(pinNumber);
            break;
        default:
            // Invalid data type
            return 4;
    }
    m_pinsType.erase(pinNumber);
    m_pinsCounter--;

    return 0;
}

std::string Device::getPin(pinId pinNumber) {
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

pinsQuantity Device::pinsCreated() {
    return m_pinsCounter;
}

pinsQuantity Device::getMaxPins() {
    return m_maxPins;
}

// Getters for pins maps
const pinsTypeMap& Device::getPinsTypes() const {
    return m_pinsType;
}

const pinsIntMap& Device::getIntPins() const {
    return m_intPins;
}

const pinsDoubleMap& Device::getDoublePins() const {
    return m_doublePins;
}

const pinsStringMap& Device::getStringPins() const {
    return m_stringPins;
}

void Device::adjustIdSequence(DeviceID id) {
    if (m_idSequence <= id)
        m_idSequence = id + 1;
}

// Device builder
Device::Builder::Builder()
    : m_outdatedDelay(500),
      m_offlineDelay(1000),
      m_maxPins(255),
      m_id(0),
      m_state(DeviceState::OFFLINE) {
}

Device::Builder& Device::Builder::setOutdatedDelay(delayMs outdatedDelay) {
    m_outdatedDelay = outdatedDelay;
    return *this;
}

Device::Builder& Device::Builder::setOfflineDelay(delayMs offlineDelay) {
    m_offlineDelay = offlineDelay;
    return *this;
}

Device::Builder& Device::Builder::setDeadDelay(delayMs deadDelay) {
    m_deadDelay = deadDelay;
    return *this;
}

Device::Builder& Device::Builder::setMaxPins(pinsQuantity maxPins) {
    m_maxPins = maxPins;
    return *this;
}

Device::Builder& Device::Builder::setID(DeviceID id) {
    m_id = id;
    return *this;
}

Device::Builder& Device::Builder::setToken(const std::string& token) {
    m_token = token;
    return *this;
}

Device::Builder& Device::Builder::setState(DeviceState state) {
    m_state = state;
    return *this;
}

Device::Builder& Device::Builder::setIntPin(pinId pinNumber, int value) {
    m_intPins[pinNumber] = value;
    setPinsTypePin(pinNumber, INTID);
    return *this;
}

Device::Builder& Device::Builder::setDoublePin(pinId pinNumber, double value) {
    m_doublePins[pinNumber] = value;
    setPinsTypePin(pinNumber, DOUBLEID);
    return *this;
}

Device::Builder& Device::Builder::setStringPin(pinId pinNumber,
                                               const std::string& value) {
    m_stringPins[pinNumber] = value;
    setPinsTypePin(pinNumber, STRINGID);
    return *this;
}

Device::Builder& Device::Builder::setPinsTypePin(pinId pinNumber,
                                                 ContainerID value) {
    m_pinsType[pinNumber] = value;
    return *this;
}

Device::Builder& Device::Builder::setIntPinMap(pinsIntMap&& intPinMap) {
    m_intPins = std::move(intPinMap);
    return *this;
}

Device::Builder& Device::Builder::setDoublePinMap(
    pinsDoubleMap&& doublePinMap) {
    m_doublePins = std::move(doublePinMap);
    return *this;
}

Device::Builder& Device::Builder::setStringPinMap(
    pinsStringMap&& stringPinMap) {
    m_stringPins = std::move(stringPinMap);
    return *this;
}

Device::Builder& Device::Builder::setPinsTypeMap(pinsTypeMap&& pinsTypeMap) {
    m_pinsType = std::move(pinsTypeMap);
    return *this;
}

Device::Builder& Device::Builder::setDeleteAfterOffline(
    bool deleteAfterOffline) {
    m_deleteAfterOffline = deleteAfterOffline;
    return *this;
}

Device Device::Builder::build() {
    Device device(m_outdatedDelay, m_offlineDelay, m_deadDelay, m_maxPins,
                  m_deleteAfterOffline);
    device.m_id = m_id;
    if (!m_token.empty())
        device.m_token = m_token;
    device.m_state = m_state;
    device.m_intPins = std::move(m_intPins);
    device.m_doublePins = std::move(m_doublePins);
    device.m_stringPins = std::move(m_stringPins);
    device.m_pinsType = std::move(m_pinsType);
    return device;
}

}  // namespace ioteye::device