#pragma once
#include <memory>

#include "utils/processorPool.hpp"

namespace ioteye::utils {

struct TimerSchedulerParams {
    size_t maxThreads = 128;
    size_t minThreads = 1;
    size_t threadCapacity = 64;
    loadt maxLoad = 80;
    loadt minLoad = 40;
    ms sleepInterval = ms(10);
};

class TimerScheduler {
public:
    explicit TimerScheduler(TimerSchedulerParams params = {});

    bool registerObject(std::shared_ptr<ManagedObject> object);
    bool removeObject(uint64_t id);
    size_t objectCount() const;

private:
    std::shared_ptr<ProcessorPool> m_pool;
};

}  // namespace ioteye::utils