#include "device.h"

ioteyeDevice::Device::StateTimer::StateTimer(std::function<void(uint8_t)> callback, std::mutex* mutex)
: 
m_changingMutex(mutex),
m_cbChangeState{callback}
{
    std::thread([this]()
    {
        while (true) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(wasPing())
            {
                m_start = std::chrono::high_resolution_clock::now();
                while (std::chrono::high_resolution_clock::now() - m_start < m_outdatedDelay)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (wasPing())
                    {
                        // m_wasPing = false;
                        // m_start = std::chrono::high_resolution_clock::now();
                        // m_callback(ioteyeDevice::Device::ONLINE);
                        break;
                    }
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
                    if (wasPing())
                    {
                        // m_start = std::chrono::high_resolution_clock::now();
                        // m_callback(ioteyeDevice::Device::ONLINE);
                        break;
                    }
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
            if(isStopped())
                break;
        }
    } ).detach();
}

void ioteyeDevice::Device::StateTimer::ping()
{
    std::lock_guard<std::mutex>lock(*m_changingMutex);
    m_wasPing = true;
}

void ioteyeDevice::Device::StateTimer::setPingFalse()
{
    std::lock_guard<std::mutex>lock(*m_changingMutex);
    m_wasPing = false;
}

bool ioteyeDevice::Device::StateTimer::wasPing()
{
    std::lock_guard<std::mutex>lock(*m_changingMutex);
    return m_wasPing;
}

void ioteyeDevice::Device::StateTimer::stop()
{
    std::lock_guard<std::mutex>lock(*m_changingMutex);
    m_isStopped = true;
}

bool ioteyeDevice::Device::StateTimer::isStopped()
{
    std::lock_guard<std::mutex>lock(*m_changingMutex);
    return m_isStopped;
}

uint64_t ioteyeDevice::Device::m_idSequence = 1;

ioteyeDevice::Device::Device()
{
    m_id = m_idSequence;
    m_idSequence++;
    m_timerMutex = new std::mutex();
}

uint8_t ioteyeDevice::Device::getState()
{
    std::lock_guard<std::mutex>lock(*m_timerMutex);
    return m_state;
}

void ioteyeDevice::Device::changeState(uint8_t state)
{
    std::lock_guard<std::mutex>lock(*m_timerMutex);
    m_state = state;
}

ioteyeDevice::Device::~Device()
{
    m_stateTimer.stop();
}
