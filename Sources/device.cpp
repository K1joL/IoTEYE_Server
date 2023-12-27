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
                setPingFalse();
                std::cout << 1 << std::endl;
                m_start = std::chrono::high_resolution_clock::now();
                while (std::chrono::high_resolution_clock::now() - m_start < m_outdatedDelay)
                {
                    std::cout << 11 << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (wasPing())
                        break;
                }
                if (wasPing())
                {
                    std::cout << 2 << std::endl;
                    m_cbChangeState(ioteyeDevice::Device::ONLINE);
                    setPingFalse();
                    continue;
                }
                // set state = outdated
                m_cbChangeState(ioteyeDevice::Device::OUTDATED);
                m_start = std::chrono::high_resolution_clock::now();
                while (std::chrono::high_resolution_clock::now() - m_start < m_offlineDelay)
                {
                    std::cout << 3 << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (wasPing())
                        break;
                }
                if (wasPing())
                {
                    std::cout << 4 << std::endl;
                    m_cbChangeState(ioteyeDevice::Device::ONLINE);
                    setPingFalse();
                    continue;
                }
                // set state = offline
                m_cbChangeState(ioteyeDevice::Device::OFFLINE);
                std::cout << 5 << std::endl;
            }
            if(isStopped())
                break;
        }
    } ).detach();
}

void ioteyeDevice::Device::StateTimer::ping()
{
    m_changingMutex->lock();
    m_wasPing = true;
    m_changingMutex->unlock();

}

void ioteyeDevice::Device::StateTimer::setPingFalse()
{
    m_changingMutex->lock();
    m_wasPing = false;
    m_changingMutex->unlock();
}

bool ioteyeDevice::Device::StateTimer::wasPing()
{
    m_changingMutex->lock();
    bool temp = m_wasPing;
    m_changingMutex->unlock();
    return temp;
}

void ioteyeDevice::Device::StateTimer::stop()
{
    m_changingMutex->lock();
    m_isStopped = true;
    m_changingMutex->unlock();
}

bool ioteyeDevice::Device::StateTimer::isStopped()
{
    m_changingMutex->lock();
    bool temp = m_isStopped;
    m_changingMutex->unlock();
    return temp;
}

uint64_t ioteyeDevice::Device::m_idSequence = 1;

ioteyeDevice::Device::Device()
{
    m_id = m_idSequence;
    m_idSequence++;
    m_timerMutex = new std::mutex();
    m_stateTimer = new StateTimer{std::bind(&Device::changeState, this, std::placeholders::_1), m_timerMutex};
}

uint8_t ioteyeDevice::Device::getState()
{
    m_timerMutex->lock();
    uint8_t temp = m_state;
    m_timerMutex->unlock();
    return temp;
}

void ioteyeDevice::Device::changeState(uint8_t state)
{
    m_timerMutex->lock();
    m_state = state;
    m_timerMutex->unlock();
}

ioteyeDevice::Device::~Device()
{
    m_stateTimer->stop();
    delete m_timerMutex;
    delete m_stateTimer;
}
