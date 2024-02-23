#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <chrono>
#include <mutex>
#include <functional.h>
#include <thread>
#include <jwt-cpp/jwt.h>

using ms = std::chrono::milliseconds;

namespace ioteyeDevice
{
    class Device
    {
        class StateTimer
        {
        public:
            StateTimer(std::function<void(uint8_t)> callback, std::mutex* mutex);
            void threadLoop();
            void ping();
            void setPingFalse();
            bool wasPing();
            void stop();
            void setDelays(ms outdatedDelay, ms offlineDelay);
            ms getRemainingTime();
            ~StateTimer(){}
            void threadStarted();
            void killThread();
            bool isThreadAlive();
        private:
            bool isStopped();
        private:
            bool m_wasPing = false;
            std::mutex *m_changingMutex = nullptr;
            ms m_outdatedDelay = ms(500);
            ms m_offlineDelay = ms(1000);
            std::chrono::high_resolution_clock::time_point m_start;
            std::function<void(uint8_t)> m_cbChangeState;
            bool m_isStopped = false;
            bool m_threadAlive = false;
        };

    public:
        Device();
        Device(uint16_t outdatedDelay, uint16_t offlineDelay, uint16_t maxPins);
        void generateToken();
        std::string getToken();
        inline uint64_t getID() { return m_id; }
        uint8_t getState();
        void changeState(uint8_t state);
        void ping() { m_stateTimer->ping(); };
        ~Device(); 

        // Virtual pins interactions
        int addPin(uint16_t pinNumber, const std::string &dataType, const std::string &defaultData);
        int changePin(uint16_t pinNumber, const std::string &data);
        int removePin(uint16_t pinNumber);
        std::string getPin(uint16_t pinNumber);

    private:
        static uint64_t m_idSequence;
        uint64_t m_id;
        std::string m_token;
        uint8_t m_state = OFFLINE;
        std::mutex *m_timerMutex = nullptr;
        StateTimer *m_stateTimer;

        // Virtual pins data type IDs
        enum ContainerID
        {
            INTID = 105,
            DOUBLEID = 100,
            STRINGID = 115
        };

        // Virtual pins data
        std::unordered_map<uint8_t, uint8_t> m_pinsType;
        std::unordered_map<uint8_t, int> m_intPins;
        std::unordered_map<uint8_t, double> m_doublePins;
        std::unordered_map<uint8_t, std::string> m_stringPins;
        uint16_t m_pinsCounter = 0;
        uint16_t m_maxPins = 255;

    public:
        enum STATES
        {
            ONLINE,
            OFFLINE,
            OUTDATED,
            MAX_STATES
        };
    };
}

#endif //!DEVICE_H