#include <utils/processorPool.hpp>

namespace ioteye::utils {
size_t ManagedObject::m_idSequence = 1;

ManagedObject::ManagedObject() {
    m_id = m_idSequence;
    ++m_idSequence;
}

objID ManagedObject::getID() const {
    return m_id;
}

void ManagedObject::process() {
}

Processor::Processor(std::chrono::milliseconds sleepInterval)
    : m_sleepInterval(sleepInterval) {
    m_thread = std::thread(&Processor::run, this);
}

Processor::~Processor() {
    stop();
    if (m_thread.joinable())
        m_thread.join();
}

Processor::Processor(Processor&& other) noexcept
    : m_objects(std::move(other.m_objects)),
      m_sleepInterval(other.m_sleepInterval),
      m_running(other.m_running.load()),
      m_isReady(other.m_isReady.load()),
      m_thread(std::move(other.m_thread)) {
}

Processor& Processor::operator=(Processor&& other) noexcept {
    if (this != &other) {
        stop();
        if (m_thread.joinable())
            m_thread.join();
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        std::lock_guard<std::shared_mutex> otherLock(other.m_mutex);
        m_objects = std::move(other.m_objects);
        m_sleepInterval = other.m_sleepInterval;
        m_running = other.m_running.load();
        m_isReady = other.m_isReady.load();
        m_thread = std::move(other.m_thread);
    }
    return *this;
}

void Processor::run() {
    while (m_running) {
        std::shared_ptr<ManagedObject> currentObj;
        {
            std::shared_lock lock(m_mutex);
            if (!m_objects.empty()) {
                static size_t lastIndex = 0;
                size_t index = lastIndex % m_objects.size();
                auto it = m_objects.begin();
                std::advance(it, index);
                currentObj = it->second;
                lastIndex++;
            }
        }

        if (currentObj) {
            currentObj->process();
        } else {
            std::this_thread::sleep_for(m_sleepInterval);
        }
    }
}

bool Processor::addObject(std::shared_ptr<ManagedObject> obj) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return m_objects.emplace(obj->getID(), obj).second;
}

bool Processor::removeObject(std::shared_ptr<ManagedObject> obj) {
    return removeObject(obj->getID());
}

bool Processor::removeObject(objID id) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return m_objects.erase(id);
}

bool Processor::moveObject(std::shared_ptr<Processor> other, objID id) {
    std::unique_lock<std::shared_mutex> sourceLock(m_mutex, std::defer_lock);
    std::unique_lock<std::shared_mutex> otherLock(other->m_mutex,
                                                  std::defer_lock);
    std::lock(sourceLock, otherLock);
    auto node = m_objects.extract(id);
    if (node)
        return other->m_objects.insert(std::move(node)).inserted;
    return false;
}

size_t Processor::getSize() const {
    std::shared_lock lock(m_mutex);
    return m_objects.size();
}

bool Processor::contains(std::shared_ptr<ManagedObject> obj) const {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return m_objects.find(obj->getID()) != m_objects.end();
}

bool Processor::contains(objID id) const {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    return m_objects.find(id) != m_objects.end();
}

void Processor::setReady(bool isReady) {
    m_isReady.store(isReady);
}

void Processor::stop() {
    m_running.store(false);
    setReady(false);
}

bool Processor::isRunning() const {
    return m_running.load();
}

bool Processor::isReady() const {
    return m_isReady.load();
}

Processor::ObjectsMap& Processor::getObjects() {
    return m_objects;
}

ProcessorPool::ProcessorPool(size_t maxProc, size_t minProc,
                             size_t procCapacity, loadt maxLoad, loadt minLoad,
                             ms sleepInterval)
    : m_maxProc(maxProc),
      m_minProc(minProc),
      m_procCapacity(procCapacity),
      m_maxLoad(maxLoad),
      m_minLoad(minLoad),
      m_sleepInterval(sleepInterval) {
    for (size_t i = 0; i < m_minProc; ++i)
        addProcessor();
}

ProcessorPool::ProcessorPool(ProcessorPool&& other)
    : m_maxProc(other.m_maxProc),
      m_minProc(other.m_minProc),
      m_procCapacity(other.m_procCapacity),
      m_maxLoad(other.m_maxLoad),
      m_minLoad(other.m_minLoad),
      m_sleepInterval(other.m_sleepInterval),
      m_procCount(other.m_procCount),
      m_objCount(other.m_objCount),
      m_processors(std::move(other.m_processors)) {
    other.m_objCount = 0;
    other.m_procCount = 0;
}

ProcessorPool& ProcessorPool::operator=(ProcessorPool&& other) {
    if (&other != this) {
        std::unique_lock<std::recursive_mutex> lock1(m_poolMutex,
                                                     std::defer_lock);
        std::unique_lock<std::recursive_mutex> lock2(other.m_poolMutex,
                                                     std::defer_lock);
        std::lock(lock1, lock2);
        m_maxLoad = other.m_maxLoad;
        m_minLoad = other.m_minLoad;
        m_maxProc = other.m_maxProc;
        m_minProc = other.m_minProc;
        m_procCapacity = other.m_procCapacity;
        m_sleepInterval = other.m_sleepInterval;
        m_objCount = other.m_objCount;
        m_procCount = other.m_procCount;
        m_processors = std::move(other.m_processors);
    }
    other.m_objCount = 0;
    other.m_procCount = 0;
    return *this;
}

ProcessorPool::~ProcessorPool() {
    stopAll();
    m_processors.clear();
}

bool ProcessorPool::registerObject(const std::shared_ptr<ManagedObject> obj) {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    if (m_objCount >= m_procCapacity * m_maxProc)
        return false;
    auto procPtr = getLeastLoadProc();
    if (procPtr != nullptr && procPtr->getSize() < m_procCapacity)
        if (procPtr->addObject(obj)) {
            ++m_objCount;
            if (!adjustProcessors())
                server::debug::logln("Error while adjusting processor count");
            return true;
        }
    return false;
}

bool ProcessorPool::removeObject(std::shared_ptr<ManagedObject> obj) {
    return removeObject(obj->getID());
}

bool ProcessorPool::removeObject(objID objId) {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    if (m_processors.empty())
        return false;
    auto foundProc = getProcessorContains(objId);
    if (foundProc != nullptr) {
        if (foundProc->removeObject(objId)) {
            --m_objCount;
            if (!adjustProcessors())
                server::debug::logln("Error while adjusting processor count");
            return true;
        }
    }
    return false;
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains(
    const std::shared_ptr<ManagedObject> obj) const {
    return getProcessorContains(obj->getID());
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains(objID id) const {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    for (size_t i = 0; i < m_procCount; ++i)
        if (m_processors[i]->contains(id))
            return m_processors[i];
    return nullptr;
}

void ProcessorPool::stopAll() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    for (auto& proc : m_processors)
        if (proc->isRunning()) {
            proc->stop();
        }
}

size_t ProcessorPool::getObjCount() const {
    return m_objCount;
}

size_t ProcessorPool::getProcCount() const {
    return m_procCount;
}

bool ProcessorPool::adjustProcessors() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    loadt load = (100 * m_objCount) / (m_procCapacity * m_procCount);
    if (m_maxProc != m_procCount && load >= m_maxLoad) {
        return addProcessor();
    }
    if (m_minProc != m_procCount && load < m_minLoad) {
        return removeProcessor();
    }
    return true;
}

bool ProcessorPool::addProcessor() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    if (m_procCount < m_maxProc) {
        try {
            m_processors.push_back(
                std::make_shared<Processor>(m_sleepInterval));
        } catch (std::exception& e) {
            server::debug::logln("Error while emplace processor: ", e.what());
            return false;
        }
        ++m_procCount;
        return true;
    }
    return false;
}

bool ProcessorPool::removeProcessor() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    if (m_procCount <= m_minProc)
        return false;
    size_t leastLoadProcId = getLeastLoadProcId();
    auto proc = getProc(leastLoadProcId);
    proc->setReady(false);
    if (redistributeObjects(proc)) {
        --m_procCount;
        m_processors.erase(m_processors.begin() + leastLoadProcId);
        return true;
    }
    return false;
}

bool ProcessorPool::redistributeObjects(std::shared_ptr<Processor> processor) {
    std::unique_lock<std::recursive_mutex> poolLock(m_poolMutex);
    auto objectsToRedistribute = processor->getObjects();
    for (auto objPair : objectsToRedistribute) {
        auto targetProc = getLeastLoadProc();
        if (!processor->moveObject(targetProc, objPair.second->getID())) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<Processor> ProcessorPool::getLeastLoadProc() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    return m_processors[getLeastLoadProcId()];
}

std::shared_ptr<Processor> ProcessorPool::getProc(size_t id) {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    if (id < m_procCount)
        return m_processors[id];
    else
        return nullptr;
}

size_t ProcessorPool::getLeastLoadProcId() {
    std::unique_lock<std::recursive_mutex> lock(m_poolMutex);
    uint8_t leastLoad = 255;
    size_t leastLoadProcId = 0;
    for (size_t i = 0; i < m_processors.size(); ++i) {
        uint8_t curLoad = 100 * m_processors[i]->getSize() / m_procCapacity;
        if (curLoad < leastLoad && m_processors[i]->isReady()) {
            leastLoad = curLoad;
            leastLoadProcId = i;
        }
    }
    return leastLoadProcId;
}

std::vector<std::shared_ptr<Processor>> ProcessorPool::getProcessors() {
    return m_processors;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMinimumProcessors(
    size_t minProc) {
    m_minProc = minProc;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setMaximumProcessors(
    size_t maxProc) {
    m_maxProc = maxProc;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setProcessorsCapacity(
    size_t procCapacity) {
    m_procCapacity = procCapacity;
    return *this;
}

ProcessorPool::Builder& ProcessorPool::Builder::setSleepInterval(
    ms sleepInterval) {
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
    ProcessorPool pool(m_maxProc, m_minProc, m_procCapacity, m_maxLoad,
                       m_minLoad, m_sleepInterval);
    return pool;
}

}  // namespace ioteye::utils