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

struct DeviceParams {
    std::string token = "token";
    types::DeviceID id = 0;
    std::array<types::DelayMs, 3> delays{0, 0, 0};
    bool deleteAfterDeath = false;
    types::PinsTypeMap pinsTypes;
    types::PinsStringMap stringPins;
    types::PinsIntMap intPins;
    types::PinsDoubleMap doublePins;
    types::PinsQuantity maxPins = 0;
};

class Device : utils::ManagedObject {
public:
    Device();
    Device(types::DelayMs outdatedDelay, types::DelayMs offlineDelay,
           types::DelayMs deadDelay, types::PinsQuantity maxPins,
           bool deleteAfterOffline);
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;
    void generateToken();
    std::string getToken() const;
    types::DeviceID getID() const;
    types::DeviceState getState() const;
    void changeState(types::DeviceState state);
    void ping();
    ~Device();

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

private:
    void adjustIdSequence(types::DeviceID id);

private:
    class StateTimer {
    public:
        StateTimer(std::function<void(types::DeviceState)> callback,
                   std::mutex& mutex);
        StateTimer(std::function<void(types::DeviceState)> callback,
                   std::mutex& mutex, types::ms offlineDelay,
                   types::ms outdatedDelay);
        ~StateTimer();
        void threadLoop();
        void ping();
        void setPingFalse();
        bool wasPing();
        void stop();
        void setDelays(types::ms outdatedDelay, types::ms offlineDelay,
                       types::ms deadDelay);
        types::ms getRemainingTime() const;
        bool isStopped() const;
        void changeState(types::DeviceState newState);
        types::ms getOfflineDelay();
        types::ms getOutdatedDelay();

    private:
        std::atomic<bool> m_wasPing{false};
        std::mutex& m_changingMutex;
        types::ms m_outdatedDelay = types::ms(500);
        types::ms m_offlineDelay = types::ms(1000);
        types::ms m_deadDelay = types::ms(10000);
        types::ChronoClock::time_point m_start = types::ChronoClock::now();
        std::function<void(types::DeviceState)> m_cbChangeState;
        std::atomic<bool> m_isStopped{false};
        std::thread m_timerThread;
        // TODO: make currentState be set to the default value
        types::DeviceState m_currentState = types::OFFLINE;
    };

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
        Builder& setIntPinMap(
            std::unordered_map<types::PinId, int>&& intPinMap);
        Builder& setDoublePinMap(types::PinsDoubleMap&& doublePinMap);
        Builder& setStringPinMap(types::PinsStringMap&& stringPinMap);
        Builder& setPinsTypeMap(types::PinsTypeMap&& pinsTypeMap);
        Builder& setDeleteAfterOffline(bool deleteAfterOffline);
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
        bool m_deleteAfterOffline = false;
    };

private:
    static types::DeviceID m_idSequence;
    types::DeviceID m_id;
    std::string m_token;
    std::atomic<types::DeviceState> m_state{types::OFFLINE};
    std::mutex m_timerMutex;
    std::shared_ptr<StateTimer> m_stateTimer;

    // Virtual pins data
    std::unordered_map<types::PinId, types::ContainerID> m_pinsType;
    std::unordered_map<types::PinId, int> m_intPins;
    std::unordered_map<types::PinId, double> m_doublePins;
    std::unordered_map<types::PinId, std::string> m_stringPins;
    types::PinsQuantity m_pinsCounter = 0;
    types::PinsQuantity m_maxPins = 255;
    bool m_deleteAfterOffline = false;
    std::function<void(types::DeviceState)> m_onStateChange;
};

}  // namespace ioteye::device

#endif  // DEVICE_H