#ifndef IOTEYE_PROCESSOR_POOL_HPP
#define IOTEYE_PROCESSOR_POOL_HPP

#include <atomic>
#include <chrono>
#include <common/logging.hpp>
#include <common/types.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <utils/managedObject.hpp>
#include <vector>

namespace ioteye::utils {

class Processor {
    using ObjectsMap =
        std::unordered_map<types::objID, std::shared_ptr<utils::ManagedObject>>;

public:
    Processor(std::chrono::milliseconds sleepInterval);
    ~Processor();
    Processor(Processor&& other) noexcept;
    Processor& operator=(Processor&& other) noexcept;

    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    void run();
    bool addObject(std::shared_ptr<utils::ManagedObject> obj);
    bool removeObject(std::shared_ptr<utils::ManagedObject> obj);
    bool removeObject(types::objID id);
    bool moveObject(std::shared_ptr<Processor> other, types::objID id);
    std::vector<types::objID> getObjectIds() const;
    void setReady(bool isReady);
    void stop();

    bool contains(std::shared_ptr<utils::ManagedObject> obj) const;
    bool contains(types::objID id) const;
    size_t getSize() const;
    bool isRunning() const;
    bool isReady() const;

private:
    friend class ProcessorPool;
    ObjectsMap m_objects;
    types::ms m_sleepInterval;
    mutable std::shared_mutex m_mutex;
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_isReady{true};
    std::thread m_thread;
    size_t m_lastIndex{0};
};

/**
 * @class ProcessorPool
 * @brief A class that manages a Processors.
 *
 * @details This class is used to create and manage Processors .
 * Processors are used to process expirable objects. There are automatic load
 * managment functions in this class operable by maxLoad and minLoad.
 */
class ProcessorPool {
public:
    /**
     * @brief Default Constructor
     */
    ProcessorPool() = default;
    /**
     * @brief ProcessorPool Constructor
     *
     * @param maxProc Maximum number of threads (Processors) to
     * cefficientlyreate
     * @param minProc Minimum number of worker threads (Processors)
     * @param procCapacity Maximum objects per thread
     * @param maxLoad Maximum percentage of objects to process
     * @param minLoad Minimum percentage of objects to process
     * @param sleepInterval The time to check the status of objects
     */
    ProcessorPool(size_t maxProc, size_t minProc, size_t procCapacity,
                  types::loadt maxLoad, types::loadt minLoad,
                  types::ms sleepInterval);
    /**
     * @brief Copy Constructor
     */
    ProcessorPool(ProcessorPool&& other);
    /**
     * @brief Assignment operator
     */
    ProcessorPool& operator=(ProcessorPool&& other);
    /**
     * @brief ProcessorPool destructor
     *
     * Stops all processors.
     */
    ~ProcessorPool();

    bool registerObject(std::shared_ptr<utils::ManagedObject> obj);
    bool removeObject(std::shared_ptr<utils::ManagedObject> obj);
    bool removeObject(types::objID id);
    std::shared_ptr<Processor> getProcessorContains(
        const std::shared_ptr<utils::ManagedObject> obj) const;
    std::shared_ptr<Processor> getProcessorContains(types::objID id) const;
    void stopAll();
    size_t getObjCount() const;
    size_t getProcCount() const;

protected:
    std::shared_ptr<Processor> getProcessorContains_nolock(types::objID id) const;
    bool adjustProcessors_nolock();
    bool addProcessor_nolock();
    bool removeProcessor_nolock();
    bool redistributeObjects_nolock(const std::shared_ptr<Processor>& processor);
    std::shared_ptr<Processor> getLeastLoadProc_nolock() const;
    std::shared_ptr<Processor> getProc_nolock(size_t id) const;
    size_t getLeastLoadProcId_nolock() const;
    std::vector<std::shared_ptr<Processor>> getProcessors_nolock() const;

public:
    class Builder {
    public:
        Builder() = default;
        Builder& setMinimumProcessors(size_t minProc);
        Builder& setMaximumProcessors(size_t maxProc);
        Builder& setProcessorsCapacity(size_t procCapacity);
        Builder& setSleepInterval(types::ms sleepInterval);
        Builder& setMaximumLoad(types::loadt maxLoad);
        Builder& setMinimumLoad(types::loadt minLoad);
        ProcessorPool build();

    private:
        size_t m_maxProc = 128;
        size_t m_minProc = 1;
        size_t m_procCapacity = 64;
        types::loadt m_maxLoad = 80;
        types::loadt m_minLoad = 40;
        types::ms m_sleepInterval = types::ms(10);
    };

private:
    // External

    /// @brief Maximum number of worker threads for this pool
    size_t m_maxProc = 128;
    /// @brief Minimum number of worker threads
    size_t m_minProc = 1;
    /// @brief Maximum objects per thread
    size_t m_procCapacity = 64;
    /// @brief Maximum percentage of objects to process. If greater, thread will
    /// be created.
    types::loadt m_maxLoad = 80;
    /// @brief Minimum percentage of objects to process. If less, thread will be
    /// removed and objects redistributed.
    types::loadt m_minLoad = 40;
    /// @brief Sleep time after object processing
    types::ms m_sleepInterval = types::ms(10);
    // Internal

    /// @brief Processor counter
    std::atomic<size_t> m_procCount = 0;
    /// @brief Object counter
    std::atomic<size_t> m_objCount = 0;
    /// @brief Vector of processors currently in use
    std::vector<std::shared_ptr<Processor>> m_processors;
    /// @brief Mutex to sync pool operations
    mutable std::mutex m_poolMutex;
};

}  // namespace ioteye::utils

#endif  // !IOTEYE_PROCESSOR_POOL_HPP