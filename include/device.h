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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace ioteye {
using ms = std::chrono::milliseconds;
using chronoClock = std::chrono::high_resolution_clock;
// Forward declaration. For using.
class Device;
using DevicePtr = std::shared_ptr<ioteye::Device>;

class Device {
    class StateTimer {
    public:
        StateTimer(std::function<void(uint8_t)> callback, std::mutex& mutex);
        ~StateTimer();
        void threadLoop();
        void ping();
        void setPingFalse();
        bool wasPing();
        void stop();
        void setDelays(ms outdatedDelay, ms offlineDelay);
        ms getRemainingTime() const;
        bool isStopped() const;
        void changeState(uint8_t newState);

    private:
        std::atomic<bool> m_wasPing{false};
        std::mutex& m_changingMutex;
        ms m_outdatedDelay = ms(500);
        ms m_offlineDelay = ms(1000);
        chronoClock::time_point m_start = chronoClock::now();
        std::function<void(uint8_t)> m_cbChangeState;
        std::atomic<bool> m_isStopped{false};
        std::thread m_timerThread;
        // TODO: make currentState be set to the default value
        uint8_t m_currentState = OFFLINE;
    };

public:
    class Builder {
    public:
        Builder();
        Builder& setOutdatedDelay(uint16_t outdatedDelay);
        Builder& setOfflineDelay(uint16_t offlineDelay);
        Builder& setMaxPins(uint16_t maxPins);
        Builder& setID(uint64_t id);
        Builder& setToken(const std::string& token);
        Builder& setState(uint8_t state);
        Builder& setIntPin(uint16_t pinNumber, int value);
        Builder& setDoublePin(uint16_t pinNumber, double value);
        Builder& setStringPin(uint16_t pinNumber, const std::string& value);
        Builder& setPinsTypePin(uint16_t pinNumber, uint8_t value);
        Builder& setIntPinMap(std::unordered_map<uint16_t, int>&& intPinMap);
        Builder& setDoublePinMap(
            std::unordered_map<uint16_t, double>&& doublePinMap);
        Builder& setStringPinMap(
            std::unordered_map<uint16_t, std::string>&& stringPinMap);
        Builder& setPinsTypeMap(
            std::unordered_map<uint16_t, uint8_t>&& pinsTypeMap);

        Device build();

    private:
        uint16_t m_outdatedDelay;
        uint16_t m_offlineDelay;
        uint16_t m_maxPins;
        uint64_t m_id;
        std::string m_token;
        uint8_t m_state;
        std::unordered_map<uint16_t, uint8_t> m_pinsType;
        std::unordered_map<uint16_t, int> m_intPins;
        std::unordered_map<uint16_t, double> m_doublePins;
        std::unordered_map<uint16_t, std::string> m_stringPins;
    };

public:
    Device();
    Device(uint16_t outdatedDelay, uint16_t offlineDelay, uint16_t maxPins);
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;
    void generateToken();
    std::string getToken();
    inline uint64_t getID() {
        return m_id;
    }
    uint8_t getState();
    void changeState(uint8_t state);
    void ping() {
        m_stateTimer->ping();
    };
    ~Device();

    // Virtual pins interactions
    int addPin(uint16_t pinNumber, const std::string& dataType,
               const std::string& defaultData);
    int changePin(uint16_t pinNumber, const std::string& data);
    int removePin(uint16_t pinNumber);
    std::string getPin(uint16_t pinNumber);
    uint16_t pinsCreated();
    uint16_t getMaxPins();
    // Getters for pins maps
    const std::unordered_map<uint16_t, uint8_t>& getPinsTypes() const;
    const std::unordered_map<uint16_t, int>& getIntPins() const;
    const std::unordered_map<uint16_t, double>& getDoublePins() const;
    const std::unordered_map<uint16_t, std::string>& getStringPins() const;

    // Virtual pins data type IDs
    enum ContainerID { INTID = 105, DOUBLEID = 100, STRINGID = 115 };

private:
    static uint64_t m_idSequence;
    uint64_t m_id;
    std::string m_token;
    std::atomic<uint8_t> m_state{OFFLINE};
    std::mutex m_timerMutex;
    std::shared_ptr<StateTimer> m_stateTimer;

    // Virtual pins data
    std::unordered_map<uint16_t, uint8_t> m_pinsType;
    std::unordered_map<uint16_t, int> m_intPins;
    std::unordered_map<uint16_t, double> m_doublePins;
    std::unordered_map<uint16_t, std::string> m_stringPins;
    uint16_t m_pinsCounter = 0;
    uint16_t m_maxPins = 255;

public:
    enum STATES { ONLINE, OFFLINE, OUTDATED, MAX_STATES };
};
}  // namespace ioteye

#endif  //! DEVICE_H