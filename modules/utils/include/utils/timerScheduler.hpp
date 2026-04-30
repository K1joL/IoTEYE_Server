#pragma once
#include <common/types.hpp>
#include <memory>
#include <utils/managedObject.hpp>
#include <utils/processorPool.hpp>

namespace ioteye::utils {

struct TimerSchedulerParams {
    size_t maxThreads = 128;
    size_t minThreads = 1;
    size_t threadCapacity = 64;
    types::loadt maxLoad = 80;
    types::loadt minLoad = 40;
    types::ms sleepInterval = types::ms(10);
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