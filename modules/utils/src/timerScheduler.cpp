#include "utils/timerScheduler.hpp"

namespace ioteye::utils {

TimerScheduler::TimerScheduler(TimerSchedulerParams p) {
    m_pool = std::make_shared<ProcessorPool>(
        ProcessorPool::Builder()
            .setMaximumProcessors(p.maxThreads)
            .setMinimumProcessors(p.minThreads)
            .setProcessorsCapacity(p.threadCapacity)
            .setMaximumLoad(p.maxLoad)
            .setMinimumLoad(p.minLoad)
            .setSleepInterval(p.sleepInterval)
            .build());
}

bool TimerScheduler::registerObject(std::shared_ptr<ManagedObject> object) {
    return m_pool->registerObject(std::move(object));
}

bool TimerScheduler::removeObject(uint64_t id) {
    return m_pool->removeObject(id);
}

size_t TimerScheduler::objectCount() const {
    return m_pool->getObjCount();
}

}  // namespace ioteye::utils