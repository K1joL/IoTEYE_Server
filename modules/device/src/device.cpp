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

std::atomic<DeviceID> Device::m_idSequence{1};

Device::Device() {
    m_id = m_idSequence.fetch_add(1, std::memory_order_relaxed);
    generateToken();
}

Device::Device(DelayMs outdatedDelay, DelayMs offlineDelay, DelayMs deadDelay, PinsQuantity maxPins,
               bool deleteAfterDeath)
    : m_outdatedDelay(outdatedDelay),
      m_offlineDelay(offlineDelay),
      m_deadDelay(deadDelay),
      m_maxPins(maxPins),
      m_deleteAfterDeath(deleteAfterDeath) {
    m_state = DeviceState::ONLINE;
    m_id = m_idSequence.fetch_add(1, std::memory_order_relaxed);
    generateToken();
}

Device::Device(Device&& other) noexcept
    : m_id(other.m_id),
      m_token(std::move(other.m_token)),
      m_state(other.m_state.load()),
      m_outdatedDelay(other.m_outdatedDelay),
      m_offlineDelay(other.m_offlineDelay),
      m_deadDelay(other.m_deadDelay),
      m_pinsType(std::move(other.m_pinsType)),
      m_intPins(std::move(other.m_intPins)),
      m_doublePins(std::move(other.m_doublePins)),
      m_stringPins(std::move(other.m_stringPins)),
      m_pinsCounter(other.m_pinsCounter),
      m_maxPins(other.m_maxPins),
      m_deleteAfterDeath(other.m_deleteAfterDeath) {
}

Device& Device::operator=(Device&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    m_id = other.m_id;
    m_token = std::move(other.m_token);
    m_state.store(other.m_state.load());
    m_outdatedDelay = other.m_outdatedDelay;
    m_offlineDelay = other.m_offlineDelay;
    m_deadDelay = other.m_deadDelay;
    m_maxPins = other.m_maxPins;
    m_pinsType = std::move(other.m_pinsType);
    m_intPins = std::move(other.m_intPins);
    m_doublePins = std::move(other.m_doublePins);
    m_stringPins = std::move(other.m_stringPins);
    m_pinsCounter = other.m_pinsCounter;
    m_deleteAfterDeath = other.m_deleteAfterDeath;

    adjustIdSequence(other.m_id);

    return *this;
}

void Device::generateToken() {
    auto token = jwt::create()
                     .set_type("JWS")
                     .set_payload_claim("deviceID", jwt::claim(std::to_string(m_id)))
                     .sign(jwt::algorithm::hs256{std::to_string(65 + m_id % 26)});
    m_token = token;
}

std::string Device::getToken() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_token;
}

DeviceID Device::getDeviceID() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_id;
}

DeviceState Device::getState() const {
    return m_state.load();
}

void Device::changeState(DeviceState state) {
    {
        std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
        if (state != m_state.load()) {
            m_state.store(state);
            // log("Changed State to ", char(state + 48));
        }
    }
    if (m_onStateChange)
        m_onStateChange(state);
}

void Device::ping() {
    m_wasPing.store(true);
}

Device::~Device() {
    // log("device desctructor");
}

void Device::setDelays(types::DelayMs outdated, types::DelayMs offline, types::DelayMs dead) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    m_outdatedDelay = types::ms(outdated);
    m_offlineDelay = types::ms(offline);
    m_deadDelay = types::ms(dead);
}

void Device::setOnStateChange(std::function<void(DeviceState)> cb) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    m_onStateChange = std::move(cb);
}

int Device::addPin(PinId pinNumber, const std::string& dataType, const std::string& defaultData) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    // the number of pins must be less than m_maxPins
    if (m_pinsCounter == m_maxPins) {
        log(LogLevel::WARNING, "the number of pins must be less than ", m_maxPins);
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
    m_pinsType.emplace(pinNumber, static_cast<ContainerID>(std::tolower(dataType[0])));
    m_pinsCounter++;

    // OK
    return 0;
}

int Device::changePin(PinId pinNumber, const std::string& data) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
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

int Device::removePin(PinId pinNumber) {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
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

std::string Device::getPin(PinId pinNumber) const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
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

PinsQuantity Device::pinsCreated() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_pinsCounter;
}

PinsQuantity Device::getMaxPins() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_maxPins;
}

bool Device::isDeleteAfterDeath() const {
    return m_deleteAfterDeath;
}

// Getters for pins maps
const PinsTypeMap& Device::getPinsTypes() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_pinsType;
}

const PinsIntMap& Device::getIntPins() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_intPins;
}

const PinsDoubleMap& Device::getDoublePins() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_doublePins;
}

const PinsStringMap& Device::getStringPins() const {
    std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
    return m_stringPins;
}

void Device::process() {
    bool hadPing = m_wasPing.exchange(false);
    auto now = ChronoClock::now();

    if (hadPing) {
        std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
        m_start = now;
        lock.unlock();
        changeState(DeviceState::ONLINE);
    }

    types::ChronoClock::time_point startCopy;
    types::ms outdatedDelayCopy;
    types::ms offlineDelayCopy;
    types::ms deadDelayCopy;
    {
        std::shared_lock<std::shared_mutex> lock(m_sharedMutex);
        startCopy = m_start;
        outdatedDelayCopy = m_outdatedDelay;
        offlineDelayCopy = m_offlineDelay;
        deadDelayCopy = m_deadDelay;
    }

    auto elapsed = std::chrono::duration_cast<ms>(now - startCopy);
    DeviceState state = m_state.load();

    switch (state) {
        case DeviceState::ONLINE:
            if (elapsed >= outdatedDelayCopy) {
                changeState(DeviceState::OUTDATED);
            }
            break;
        case DeviceState::OUTDATED:
            if (elapsed >= offlineDelayCopy) {
                changeState(DeviceState::OFFLINE);
            }
            break;
        case DeviceState::OFFLINE:
            if (elapsed >= deadDelayCopy) {
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

void Device::adjustIdSequence(DeviceID id) {
    DeviceID currentId = m_idSequence.load(std::memory_order_relaxed);
    while (currentId <= id &&
           !m_idSequence.compare_exchange_weak(currentId, id + 1, std::memory_order_relaxed,
                                               std::memory_order_relaxed)) {
        // loop until successful
    }
}

Device::Device(DeviceID id, DelayMs outdatedDelay, DelayMs offlineDelay, DelayMs deadDelay,
               PinsQuantity maxPins, bool deleteAfterDeath)
    : m_id(id),
      m_outdatedDelay(outdatedDelay),
      m_offlineDelay(offlineDelay),
      m_deadDelay(deadDelay),
      m_maxPins(maxPins),
      m_deleteAfterDeath(deleteAfterDeath) {
    generateToken();
}

// Device builder
Device::Builder::Builder()
    : m_outdatedDelay(500),
      m_offlineDelay(1000),
      m_maxPins(255),
      m_id(0),
      m_state(DeviceState::OFFLINE) {
}

Device::Builder& Device::Builder::setOutdatedDelay(DelayMs outdatedDelay) {
    m_outdatedDelay = outdatedDelay;
    return *this;
}

Device::Builder& Device::Builder::setOfflineDelay(DelayMs offlineDelay) {
    m_offlineDelay = offlineDelay;
    return *this;
}

Device::Builder& Device::Builder::setDeadDelay(DelayMs deadDelay) {
    m_deadDelay = deadDelay;
    return *this;
}

Device::Builder& Device::Builder::setMaxPins(PinsQuantity maxPins) {
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

Device::Builder& Device::Builder::setIntPin(PinId pinNumber, int value) {
    m_intPins[pinNumber] = value;
    setPinsTypePin(pinNumber, INTID);
    return *this;
}

Device::Builder& Device::Builder::setDoublePin(PinId pinNumber, double value) {
    m_doublePins[pinNumber] = value;
    setPinsTypePin(pinNumber, DOUBLEID);
    return *this;
}

Device::Builder& Device::Builder::setStringPin(PinId pinNumber, const std::string& value) {
    m_stringPins[pinNumber] = value;
    setPinsTypePin(pinNumber, STRINGID);
    return *this;
}

Device::Builder& Device::Builder::setPinsTypePin(PinId pinNumber, ContainerID value) {
    m_pinsType[pinNumber] = value;
    return *this;
}

Device::Builder& Device::Builder::setIntPinMap(PinsIntMap&& intPinMap) {
    m_intPins = std::move(intPinMap);
    return *this;
}

Device::Builder& Device::Builder::setDoublePinMap(PinsDoubleMap&& doublePinMap) {
    m_doublePins = std::move(doublePinMap);
    return *this;
}

Device::Builder& Device::Builder::setStringPinMap(PinsStringMap&& stringPinMap) {
    m_stringPins = std::move(stringPinMap);
    return *this;
}

Device::Builder& Device::Builder::setPinsTypeMap(PinsTypeMap&& pinsTypeMap) {
    m_pinsType = std::move(pinsTypeMap);
    return *this;
}

Device::Builder& Device::Builder::setDeleteAfterDeath(bool deleteAfterDeath) {
    m_deleteAfterDeath = deleteAfterDeath;
    return *this;
}

Device Device::Builder::build() {
    Device device(m_id, m_outdatedDelay, m_offlineDelay, m_deadDelay, m_maxPins,
                  m_deleteAfterDeath);
    device.m_state = m_state;
    device.m_intPins = std::move(m_intPins);
    device.m_doublePins = std::move(m_doublePins);
    device.m_stringPins = std::move(m_stringPins);
    device.m_pinsType = std::move(m_pinsType);
    return device;
}

}  // namespace ioteye