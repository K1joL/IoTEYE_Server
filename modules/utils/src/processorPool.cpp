#include <utils/processorPool.hpp>

namespace ioteye::utils {
using namespace ioteye::types;
using ioteye::server::debug::LogLevel;
using namespace ioteye::server;

Processor::Processor(std::chrono::milliseconds sleepInterval) : m_sleepInterval(sleepInterval) {
    m_thread = std::thread(&Processor::run, this);
}

Processor::~Processor() {
    stop();
    if (m_thread.joinable())
        m_thread.join();
}

Processor::Processor(Processor&& other) noexcept
    : m_objects(std::move(other.m_objects)),
      m_indexMap(std::move(other.m_indexMap)),
      m_sleepInterval(other.m_sleepInterval),
      m_running(other.m_running.load(std::memory_order_relaxed)),
      m_isReady(other.m_isReady.load(std::memory_order_relaxed)),
      m_thread(std::move(other.m_thread)),
      m_nextIndex(other.m_nextIndex) {
}

Processor& Processor::operator=(Processor&& other) noexcept {
    if (this != &other) {
        stop();
        if (m_thread.joinable())
            m_thread.join();

        std::scoped_lock lock(m_mutex, other.m_mutex);

        m_objects = std::move(other.m_objects);
        m_indexMap = std::move(other.m_indexMap);
        m_sleepInterval = other.m_sleepInterval;
        m_running.store(other.m_running.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_isReady.store(other.m_isReady.load(std::memory_order_relaxed), std::memory_order_relaxed);
        m_thread = std::move(other.m_thread);
        m_nextIndex = other.m_nextIndex;
    }
    return *this;
}

void Processor::run() {
    while (m_running.load(std::memory_order_relaxed)) {
        std::shared_ptr<ManagedObject> currentObj;
        {
            std::shared_lock lock(m_mutex);
            if (!m_objects.empty()) {
                if (m_nextIndex >= m_objects.size()) {
                    m_nextIndex = 0;
                }
                currentObj = m_objects[m_nextIndex++];
            }
        }

        if (currentObj) {
            currentObj->process();
        }
        std::this_thread::sleep_for(m_sleepInterval);
    }
}

bool Processor::addObject(std::shared_ptr<ManagedObject> obj) {
    std::unique_lock lock(m_mutex);
    auto id = obj->getObjID();

    if (m_indexMap.contains(id)) {
        return false;
    }

    m_indexMap[id] = m_objects.size();
    m_objects.push_back(std::move(obj));
    return true;
}

bool Processor::removeObject(std::shared_ptr<ManagedObject> obj) {
    return obj ? removeObject(obj->getObjID()) : false;
}

bool Processor::removeObject(types::ObjID id) {
    std::unique_lock lock(m_mutex);
    auto it = m_indexMap.find(id);
    if (it == m_indexMap.end()) {
        return false;
    }

    size_t idx = it->second;
    size_t last_idx = m_objects.size() - 1;

    if (idx != last_idx) {
        auto last_id = m_objects[last_idx]->getObjID();
        m_objects[idx] = std::move(m_objects[last_idx]);
        m_indexMap[last_id] = idx;
    }

    m_objects.pop_back();
    m_indexMap.erase(it);
    return true;
}

bool Processor::moveObject(std::shared_ptr<Processor> other, types::ObjID id) {
    std::unique_lock sourceLock(m_mutex, std::defer_lock);
    std::unique_lock otherLock(other->m_mutex, std::defer_lock);
    std::scoped_lock lock(sourceLock, otherLock);

    auto it = m_indexMap.find(id);
    if (it == m_indexMap.end() || other->m_indexMap.contains(id)) {
        return false;
    }

    auto obj = std::move(m_objects[it->second]);

    size_t idx = it->second;
    size_t last_idx = m_objects.size() - 1;

    if (idx != last_idx) {
        auto last_id = m_objects[last_idx]->getObjID();
        m_objects[idx] = std::move(m_objects[last_idx]);
        m_indexMap[last_id] = idx;
    }

    m_objects.pop_back();
    m_indexMap.erase(it);

    other->m_indexMap[id] = other->m_objects.size();
    other->m_objects.push_back(std::move(obj));

    return true;
}

size_t Processor::getSize() const {
    std::shared_lock lock(m_mutex);
    return m_objects.size();
}

bool Processor::contains(std::shared_ptr<ManagedObject> obj) const {
    return obj ? contains(obj->getObjID()) : false;
}

bool Processor::contains(types::ObjID id) const {
    std::shared_lock lock(m_mutex);
    return m_indexMap.contains(id);
}

std::vector<types::ObjID> Processor::getObjectIds() const {
    std::shared_lock lock(m_mutex);
    std::vector<types::ObjID> ids;
    ids.reserve(m_objects.size());
    for (const auto& ptr : m_objects) {
        ids.push_back(ptr->getObjID());
    }
    return ids;
}

void Processor::setReady(bool isReady) {
    m_isReady.store(isReady, std::memory_order_relaxed);
}

void Processor::stop() {
    m_running.store(false, std::memory_order_relaxed);
    setReady(false);
}

bool Processor::isRunning() const {
    return m_running.load(std::memory_order_relaxed);
}

bool Processor::isReady() const {
    return m_isReady.load(std::memory_order_relaxed);
}

ProcessorPool::ProcessorPool() {
    for (size_t i = 0; i < m_minProc; ++i)
        addProcessor_nolock();
}

ProcessorPool::ProcessorPool(size_t maxProc, size_t minProc, size_t procCapacity, loadt maxLoad,
                             loadt minLoad, ms sleepInterval)
    : m_maxProc(maxProc),
      m_minProc(minProc),
      m_procCapacity(procCapacity),
      m_maxLoad(maxLoad),
      m_minLoad(minLoad),
      m_sleepInterval(sleepInterval) {
    for (size_t i = 0; i < m_minProc; ++i)
        addProcessor_nolock();
}

ProcessorPool::ProcessorPool(ProcessorPool&& other)
    : m_maxProc(other.m_maxProc),
      m_minProc(other.m_minProc),
      m_procCapacity(other.m_procCapacity),
      m_maxLoad(other.m_maxLoad),
      m_minLoad(other.m_minLoad),
      m_sleepInterval(other.m_sleepInterval),
      m_processors(std::move(other.m_processors)) {
    m_procCount.store(other.m_procCount.load());
    m_objCount.store(other.m_objCount.load());
    other.m_objCount.store(0);
    other.m_procCount.store(0);
}

ProcessorPool& ProcessorPool::operator=(ProcessorPool&& other) {
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(m_poolMutex, other.m_poolMutex);
    m_maxLoad = other.m_maxLoad;
    m_minLoad = other.m_minLoad;
    m_maxProc = other.m_maxProc;
    m_minProc = other.m_minProc;
    m_procCapacity = other.m_procCapacity;
    m_sleepInterval = other.m_sleepInterval;
    m_processors = std::move(other.m_processors);

    m_procCount.store(other.m_procCount.load());
    m_objCount.store(other.m_objCount.load());
    other.m_objCount.store(0);
    other.m_procCount.store(0);

    return *this;
}

ProcessorPool::~ProcessorPool() {
    stopAll();
    m_processors.clear();
}

bool ProcessorPool::registerObject(const std::shared_ptr<ManagedObject> obj) {
    std::lock_guard<std::mutex> lock(m_poolMutex);

    if (m_objCount >= m_procCapacity * m_maxProc) {
        return false;
    }

    auto procPtr = getLeastLoadProc_nolock();
    if (!procPtr || procPtr->getSize() >= m_procCapacity) {
        return false;
    }

    if (!procPtr->addObject(obj)) {
        return false;
    }

    m_objCount.fetch_add(1);
    bool result = adjustProcessors_nolock();
    if (!result)
        debug::logln(LogLevel::ERROR, "Error while adjusting processor count");

    return true;
}

bool ProcessorPool::removeObject(std::shared_ptr<ManagedObject> obj) {
    if (!obj)
        return false;
    return removeObject(obj->getObjID());
}

bool ProcessorPool::removeObject(ObjID objId) {
    std::lock_guard<std::mutex> lock(m_poolMutex);

    if (m_processors.empty()) {
        return false;
    }

    auto foundProc = getProcessorContains_nolock(objId);
    if (foundProc) {
        if (foundProc->removeObject(objId)) {
            m_objCount.fetch_sub(1);
            if (!adjustProcessors_nolock())
                debug::logln(LogLevel::ERROR, "Error while adjusting processor count");
            return true;
        }
    }
    return false;
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains(
    const std::shared_ptr<ManagedObject> obj) const {
    std::lock_guard<std::mutex> lock(m_poolMutex);
    return getProcessorContains_nolock(obj->getObjID());
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains(ObjID id) const {
    std::lock_guard<std::mutex> lock(m_poolMutex);
    return getProcessorContains_nolock(id);
}

void ProcessorPool::stopAll() {
    std::lock_guard<std::mutex> lock(m_poolMutex);
    for (auto& proc : m_processors)
        if (proc->isRunning()) {
            proc->stop();
        }
}

size_t ProcessorPool::getObjCount() const {
    return m_objCount.load();
}

size_t ProcessorPool::getProcCount() const {
    return m_procCount.load();
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains_nolock(ObjID id) const {
    for (size_t i = 0; i < m_processors.size(); ++i)
        if (m_processors[i]->contains(id))
            return m_processors[i];
    return nullptr;
}

bool ProcessorPool::adjustProcessors_nolock() {
    auto procCount = m_procCount.load();
    if (procCount == 0 || m_procCapacity == 0) {
        return false;
    }

    loadt load = (100 * m_objCount.load()) / (m_procCapacity * procCount);
    if (m_maxProc != procCount && load >= m_maxLoad) {
        return addProcessor_nolock();
    }

    if (m_minProc != procCount && load < m_minLoad) {
        return removeProcessor_nolock();
    }

    return true;
}

bool ProcessorPool::addProcessor_nolock() {
    if (m_procCount.load() < m_maxProc) {
        try {
            m_processors.push_back(std::make_shared<Processor>(m_sleepInterval));
        } catch (std::exception& e) {
            debug::logln(LogLevel::ERROR, "Error while emplace processor: ", e.what());
            return false;
        }
        m_procCount.fetch_add(1);
        return true;
    }
    return false;
}

bool ProcessorPool::removeProcessor_nolock() {
    if (m_procCount.load() <= m_minProc)
        return false;

    size_t leastLoadProcId = getLeastLoadProcId_nolock();
    auto proc = getProc_nolock(leastLoadProcId);
    if (!proc) {
        return false;
    }
    // turn off processor first
    proc->setReady(false);
    // trying to redistribute
    // set the ready flag back to true in case of failure
    if (!redistributeObjects_nolock(proc)) {
        proc->setReady(true);
        return false;
    }
    // successful redistribution
    m_procCount.fetch_sub(1);
    m_processors.erase(m_processors.begin() + leastLoadProcId);
    return true;
}

bool ProcessorPool::redistributeObjects_nolock(const std::shared_ptr<Processor>& processor) {
    auto ids = processor->getObjectIds();

    for (auto id : ids) {
        if (!processor->contains(id)) {
            continue;
        }

        auto targetProc = getLeastLoadProc_nolock();
        if (!targetProc || targetProc == processor) {
            return false;
        }

        if (!processor->moveObject(targetProc, id)) {
            if (!processor->contains(id)) {
                continue;
            }
            return false;
        }
    }

    return true;
}

std::shared_ptr<Processor> ProcessorPool::getLeastLoadProc_nolock() const {
    if (m_processors.empty())
        return nullptr;
    auto id = getLeastLoadProcId_nolock();
    if (id >= m_processors.size())
        return nullptr;
    return m_processors[id];
}

std::shared_ptr<Processor> ProcessorPool::getProc_nolock(size_t id) const {
    if (id < m_procCount.load())
        return m_processors[id];
    else
        return nullptr;
}

size_t ProcessorPool::getLeastLoadProcId_nolock() const {
    uint8_t leastLoad = 255;
    size_t leastLoadProcId = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < m_processors.size(); ++i) {
        if (!m_processors[i] || !m_processors[i]->isReady()) {
            continue;
        }

        uint8_t curLoad = 100 * m_processors[i]->getSize() / m_procCapacity;
        if (curLoad < leastLoad) {
            leastLoad = curLoad;
            leastLoadProcId = i;
        }
    }
    return leastLoadProcId;
}

std::vector<std::shared_ptr<Processor>> ProcessorPool::getProcessors_nolock() const {
    return m_processors;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMinimumProcessors(size_t minProc) {
    m_minProc = minProc;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMaximumProcessors(size_t maxProc) {
    m_maxProc = maxProc;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setProcessorsCapacity(size_t procCapacity) {
    m_procCapacity = procCapacity;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setSleepInterval(ms sleepInterval) {
    m_sleepInterval = sleepInterval;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMaximumLoad(loadt maxLoad) {
    m_maxLoad = maxLoad;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMinimumLoad(loadt minLoad) {
    m_minLoad = minLoad;
    return *this;
}

ProcessorPool ProcessorPool::Builder::build() {
    ProcessorPool pool(m_maxProc, m_minProc, m_procCapacity, m_maxLoad, m_minLoad, m_sleepInterval);
    return pool;
}

}  // namespace ioteye::utils