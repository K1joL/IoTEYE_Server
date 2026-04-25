#include <utils/processorPool.hpp>

namespace ioteye {
size_t ManagedObject::m_idSequence = 1;

ManagedObject::ManagedObject() {
    m_id = m_idSequence;
    ++m_idSequence;
}

objID ManagedObject::getID() const {
    return m_id;
}

void ManagedObject::process() {
    return;
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
    std::unique_lock<std::shared_mutex> lock(m_mutex);
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

ProcessorPool::ProcessorPool(size_t minProc, size_t maxProc,
                             size_t procCapacity, ms sleepInterval,
                             loadt maxLoad, loadt minLoad)
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
      m_procCount(other.m_procCount),
      m_objCount(other.m_objCount),
      m_maxLoad(other.m_maxLoad),
      m_minLoad(other.m_minLoad),
      m_sleepInterval(other.m_sleepInterval),
      m_processors(std::move(other.m_processors)),
      m_threads(std::move(other.m_threads)) {
}

ProcessorPool& ProcessorPool::operator=(ProcessorPool&& other) {
    if (&other != this) {
        m_maxLoad = other.m_maxLoad;
        m_minLoad = other.m_minLoad;
        m_maxProc = other.m_maxProc;
        m_minProc = other.m_minProc;
        m_procCapacity = other.m_procCapacity;
        m_sleepInterval = other.m_sleepInterval;
        m_objCount = other.m_objCount;
        m_procCount = other.m_procCount;
        m_processors = std::move(other.m_processors);
        m_threads = std::move(other.m_threads);
    }
    return *this;
}

ProcessorPool::~ProcessorPool() {
    stopAll();
    m_processors.clear();
}

bool ProcessorPool::registerObject(const std::shared_ptr<ManagedObject> obj) {
    if (m_objCount >= m_procCapacity * m_maxProc)
        return false;
    auto procPtr = getLeastLoadProc();
    if (procPtr != nullptr && procPtr->getSize() < m_procCapacity)
        if (procPtr->addObject(obj)) {
            ++m_objCount;
            if (!adjustProcessors())
                server::debug::logln(server::debug::LogLevel::ERROR,
                                     "Error while adjusting processor count");
            return true;
        }
    return false;
}

bool ProcessorPool::removeObject(std::shared_ptr<ManagedObject> obj) {
    return removeObject(obj->getID());
}

inline bool ProcessorPool::removeObject(objID objId) {
    if (m_processors.empty())
        return false;
    auto foundProc = getProcessorContains(objId);
    if (foundProc != nullptr) {
        if (foundProc->removeObject(objId)) {
            --m_objCount;
            if (!adjustProcessors())
                server::debug::logln(server::debug::LogLevel::ERROR,
                                     "Error while adjusting processor count");
            return true;
        }
    }
    return false;
}

std::shared_ptr<Processor> ProcessorPool::getProcessorContains(
    const std::shared_ptr<ManagedObject> obj) const {
    for (size_t i = 0; i < m_procCount; ++i)
        if (m_processors[i]->contains(obj))
            return m_processors[i];
    return nullptr;
}

inline std::shared_ptr<Processor> ProcessorPool::getProcessorContains(
    objID id) const {
    return m_processors[id];
}

void ProcessorPool::stopAll() {
    for (auto& proc : m_processors)
        if (proc->isRunning())
            proc->stop();
}

bool ProcessorPool::adjustProcessors() {
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
    if (m_procCount < m_maxProc) {
        try {
            m_processors.push_back(
                std::make_shared<Processor>(m_sleepInterval));
        } catch (std::exception& e) {
            server::debug::logln(server::debug::LogLevel::ERROR,
                                 "Error while emplace processor: ", e.what());
            return false;
        }
        ++m_procCount;
        return true;
    }
    return false;
}

bool ProcessorPool::removeProcessor() {
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
    auto objectsToRedistribute = processor->getObjects();
    for (auto objPair : objectsToRedistribute) {
        if (!processor->moveObject(getLeastLoadProc(),
                                   objPair.second->getID())) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<Processor> ProcessorPool::getLeastLoadProc() {
    return m_processors[getLeastLoadProcId()];
}

inline std::shared_ptr<Processor> ProcessorPool::getProc(size_t id) {
    if (id < m_procCount)
        return m_processors[id];
    else
        return nullptr;
}

size_t ProcessorPool::getLeastLoadProcId() {
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

ProcessorPool::Builder::Builder() {
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
    ProcessorPool pool(m_minProc, m_maxProc, m_procCapacity, m_sleepInterval,
                       m_maxLoad, m_minLoad);
    return pool;
}

}  // namespace ioteye