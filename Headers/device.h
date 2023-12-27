#ifndef DEVICE_H
#define DEVICE_H

#include <cstdint>
#include <chrono>
#include <mutex>
#include <functional>
#include <thread>
#include <iostream>

namespace ioteyeDevice
{
    class Device
    {
        class StateTimer
        {
        public:
            StateTimer(std::function<void(uint8_t)> callback, std::mutex* mutex);
            void ping();
            void setPingFalse();
            bool wasPing();
            void stop();
        private:
            bool isStopped();
        private:
            bool m_wasPing = false;
            std::mutex *m_changingMutex = nullptr;
            std::chrono::milliseconds m_outdatedDelay = std::chrono::milliseconds(5);
            std::chrono::milliseconds m_offlineDelay = std::chrono::milliseconds(15);
            std::chrono::high_resolution_clock::time_point m_start;
            std::function<void(uint8_t)> m_cbChangeState;
            bool m_isStopped = false;
        };

    public:
        Device();
        uint64_t getID() { ping(); return m_id; }
        uint8_t getState();
        void changeState(uint8_t state);
        void ping() { m_stateTimer->ping(); };
        ~Device();
    private:
        static uint64_t m_idSequence;
        uint64_t m_id = 0;
        uint8_t m_state = OFFLINE;
        std::mutex *m_timerMutex = nullptr;
        StateTimer *m_stateTimer;

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