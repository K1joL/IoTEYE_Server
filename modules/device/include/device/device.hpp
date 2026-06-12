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

#ifndef DEVICE_H
#define DEVICE_H

#include <jwt-cpp/jwt.h>

#include <array>
#include <atomic>
#include <chrono>
#include <common/types.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utils/managedObject.hpp>

namespace ioteye {

class Device : public utils::ManagedObject {
public:
    Device();
    Device(types::DelayMs outdatedDelay, types::DelayMs offlineDelay,
           types::DelayMs deadDelay, types::PinsQuantity maxPins,
           bool deleteAfterDeath);
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;
    std::string getToken() const;
    types::DeviceID getDeviceID() const;
    types::DeviceState getState() const;
    void changeState(types::DeviceState state);
    void ping();
    bool isDeleteAfterDeath() const;
    ~Device();

    void setDelays(types::DelayMs outdated, types::DelayMs offline,
                   types::DelayMs dead);
    void setOnStateChange(std::function<void(types::DeviceState)> cb);
    // Virtual pins interactions
    int addPin(types::PinId pinNumber, const std::string& dataType,
               const std::string& defaultData);
    int changePin(types::PinId pinNumber, const std::string& data);
    int removePin(types::PinId pinNumber);
    std::string getPin(types::PinId pinNumber) const;
    types::PinsQuantity pinsCreated() const;
    types::PinsQuantity getMaxPins() const;
    // Getters for pins maps
    const types::PinsTypeMap& getPinsTypes() const;
    const types::PinsIntMap& getIntPins() const;
    const types::PinsDoubleMap& getDoublePins() const;
    const types::PinsStringMap& getStringPins() const;
    void process() override;

private:
    void adjustIdSequence(types::DeviceID id);
    Device(types::DeviceID id, types::DelayMs outdatedDelay,
           types::DelayMs offlineDelay, types::DelayMs deadDelay,
           types::PinsQuantity maxPins, bool deleteAfterDeath);
    void generateToken();

public:
    class Builder {
    public:
        Builder();
        Builder& setOutdatedDelay(types::DelayMs outdatedDelay);
        Builder& setOfflineDelay(types::DelayMs offlineDelay);
        Builder& setDeadDelay(types::DelayMs deadDelay);
        Builder& setMaxPins(types::PinsQuantity maxPins);
        Builder& setID(types::DeviceID id);
        Builder& setToken(const std::string& token);
        Builder& setState(types::DeviceState state);
        Builder& setIntPin(types::PinId pinNumber, int value);
        Builder& setDoublePin(types::PinId pinNumber, double value);
        Builder& setStringPin(types::PinId pinNumber, const std::string& value);
        Builder& setPinsTypePin(types::PinId pinNumber,
                                types::ContainerID value);
        Builder& setIntPinMap(types::PinsIntMap&& intPinMap);
        Builder& setDoublePinMap(types::PinsDoubleMap&& doublePinMap);
        Builder& setStringPinMap(types::PinsStringMap&& stringPinMap);
        Builder& setPinsTypeMap(types::PinsTypeMap&& pinsTypeMap);
        Builder& setDeleteAfterDeath(bool deleteAfterDeath);
        Device build();

    private:
        types::DelayMs m_outdatedDelay;
        types::DelayMs m_offlineDelay;
        types::DelayMs m_deadDelay;
        types::PinsQuantity m_maxPins;
        types::DeviceID m_id;
        std::string m_token;
        types::DeviceState m_state;
        std::unordered_map<types::PinId, types::ContainerID> m_pinsType;
        std::unordered_map<types::PinId, int> m_intPins;
        std::unordered_map<types::PinId, double> m_doublePins;
        std::unordered_map<types::PinId, std::string> m_stringPins;
        bool m_deleteAfterDeath = false;
    };

private:
    static std::atomic<types::DeviceID> m_idSequence;
    types::DeviceID m_id;
    std::string m_token;
    std::atomic<types::DeviceState> m_state{types::OFFLINE};

    std::atomic<bool> m_wasPing{false};
    types::ms m_outdatedDelay = types::ms(500);
    types::ms m_offlineDelay = types::ms(1000);
    types::ms m_deadDelay = types::ms(10000);
    types::ChronoClock::time_point m_start = types::ChronoClock::now();

    // Virtual pins data
    std::unordered_map<types::PinId, types::ContainerID> m_pinsType;
    std::unordered_map<types::PinId, int> m_intPins;
    std::unordered_map<types::PinId, double> m_doublePins;
    std::unordered_map<types::PinId, std::string> m_stringPins;
    types::PinsQuantity m_pinsCounter = 0;
    types::PinsQuantity m_maxPins = 255;
    bool m_deleteAfterDeath = false;
    std::function<void(types::DeviceState)> m_onStateChange;
};

}  // namespace ioteye

#endif  // DEVICE_H