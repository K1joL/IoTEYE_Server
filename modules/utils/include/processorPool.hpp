#ifndef IOTEYE_PROCESSOR_POOL_HPP
#define IOTEYE_PROCESSOR_POOL_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <logging.hpp>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ioteye {
using ms = std::chrono::milliseconds;
using loadt = uint8_t;
using objID = size_t;

class ManagedObject {
public:
    ManagedObject();
    objID getID() const;
    virtual void process();

protected:
    objID m_id;
    mutable std::shared_mutex m_sharedMutex;

private:
    static size_t m_idSequence;
};

class Processor {
    using ObjectsMap =
        std::unordered_map<objID, std::shared_ptr<ManagedObject>>;

public:
    Processor(std::chrono::milliseconds sleepInterval);
    ~Processor();
    Processor(Processor&& other) noexcept;
    Processor& operator=(Processor&& other) noexcept;

    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    void run();
    bool addObject(std::shared_ptr<ManagedObject> obj);
    bool removeObject(std::shared_ptr<ManagedObject> obj);
    bool removeObject(objID id);
    bool moveObject(std::shared_ptr<Processor> other, objID id);
    ObjectsMap& getObjects();
    void setReady(bool isReady);
    void stop();

    bool contains(std::shared_ptr<ManagedObject> obj) const;
    bool contains(objID id) const;
    size_t getSize() const;
    bool isRunning() const;
    bool isReady() const;

private:
    ObjectsMap m_objects;
    ms m_sleepInterval;
    mutable std::shared_mutex m_mutex;
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_isReady{true};
    std::thread m_thread;
};

class ProcessorPool {
public:
    ProcessorPool() = default;
    ProcessorPool(size_t minProc, size_t maxProc, size_t procCapacity,
                  ms sleepInterval, loadt maxLoad, loadt minLoad);
    ProcessorPool(ProcessorPool&& other);
    ProcessorPool& operator=(ProcessorPool&& other);
    ~ProcessorPool();

    bool registerObject(std::shared_ptr<ManagedObject> obj);
    bool removeObject(std::shared_ptr<ManagedObject> obj);
    bool removeObject(objID id);
    std::shared_ptr<Processor> getProcessorContains(
        const std::shared_ptr<ManagedObject> obj) const;
    std::shared_ptr<Processor> getProcessorContains(objID id) const;
    void stopAll();

private:
    bool adjustProcessors();
    bool addProcessor();
    bool removeProcessor();
    bool redistributeObjects(std::shared_ptr<Processor> processor);
    std::shared_ptr<Processor> getLeastLoadProc();
    std::shared_ptr<Processor> getProc(size_t id);
    size_t getLeastLoadProcId();

public:
    class Builder {
    public:
        Builder();
        Builder& setMinimumProcessors(size_t minProc);
        Builder& setMaximumProcessors(size_t maxProc);
        Builder& setProcessorsCapacity(size_t procCapacity);
        Builder& setSleepInterval(ms sleepInterval);
        Builder& setMaximumLoad(loadt maxLoad);
        Builder& setMinimumLoad(loadt minLoad);
        ProcessorPool build();

    private:
        size_t m_maxProc = 128;
        size_t m_minProc = 1;
        size_t m_procCapacity = 64;
        loadt m_maxLoad = 80;
        loadt m_minLoad = 40;
        ms m_sleepInterval = ms(10);
    };

private:
    size_t m_maxProc = 128;
    size_t m_minProc = 1;
    size_t m_procCapacity = 64;
    size_t m_procCount = 0;
    size_t m_objCount = 0;
    loadt m_maxLoad = 80;
    loadt m_minLoad = 40;
    ms m_sleepInterval = ms(10);
    std::vector<std::shared_ptr<Processor>> m_processors;
    std::vector<std::thread> m_threads;
};

}  // namespace ioteye

#endif  // !IOTEYE_PROCESSOR_POOL_HPP