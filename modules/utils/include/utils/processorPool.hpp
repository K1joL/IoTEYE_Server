/*# MIT License

# Copyright (c) 2025 Shults Bogdan aka K1joL

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

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

/**
 * @class Processor
 * @brief Dedicated worker thread for processing a subset of ManagedObjects.
 * @details Maintains a thread-safe collection of objects and continuously
 * invokes their processing logic. Supports dynamic object migration
 * between processors for load balancing.
 */
class Processor {
    using ObjectsMap = std::unordered_map<types::ObjID, std::shared_ptr<utils::ManagedObject>>;

public:
    /**
     * @brief Constructs a Processor and launches the worker thread.
     * @param sleepInterval Sleep duration between processing iterations.
     */
    Processor(std::chrono::milliseconds sleepInterval);

    /**
     * @brief Destructor. Stops the worker thread and blocks until it joins.
     */
    ~Processor();

    /**
     * @brief Move constructor. Transfers state and thread ownership.
     */
    Processor(Processor&& other) noexcept;

    /**
     * @brief Move assignment operator. Transfers state and thread ownership.
     */
    Processor& operator=(Processor&& other) noexcept;

    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    /**
     * @brief Main execution loop for the worker thread.
     */
    void run();

    /**
     * @brief Registers a new object to be processed by this worker.
     * @param obj Shared pointer to the managed object.
     * @return True if the object was successfully added, false if it already
     * exists.
     */
    bool addObject(std::shared_ptr<utils::ManagedObject> obj);

    /**
     * @brief Removes an object from the processing queue.
     * @param obj Shared pointer to the managed object.
     * @return True if the object was found and removed.
     */
    bool removeObject(std::shared_ptr<utils::ManagedObject> obj);

    /**
     * @brief Removes an object from the processing queue by its ID.
     * @param id Unique identifier of the object.
     * @return True if the object was found and removed.
     */
    bool removeObject(types::ObjID id);

    /**
     * @brief Atomically moves an object from this processor to another.
     * @param other Target processor to receive the object.
     * @param id Unique identifier of the object to move.
     * @return True if the object was successfully transferred.
     */
    bool moveObject(std::shared_ptr<Processor> other, types::ObjID id);

    /**
     * @brief Retrieves the IDs of all objects currently managed by this
     * processor.
     * @return Vector of object IDs.
     */
    std::vector<types::ObjID> getObjectIds() const;

    /**
     * @brief Updates the readiness state of the processor.
     * @details Used by the pool during load balancing to prevent new
     * assignments to a processor slated for removal.
     * @param isReady True if the processor can accept new objects.
     */
    void setReady(bool isReady);

    /**
     * @brief Signals the worker thread to terminate.
     */
    void stop();

    /**
     * @brief Checks if the specified object is managed by this processor.
     * @param obj Shared pointer to the managed object.
     * @return True if the object is present.
     */
    bool contains(std::shared_ptr<utils::ManagedObject> obj) const;

    /**
     * @brief Checks if an object with the specified ID is managed by this
     * processor.
     * @param id Unique identifier of the object.
     * @return True if the object is present.
     */
    bool contains(types::ObjID id) const;

    /**
     * @brief Returns the number of objects currently managed.
     * @return Current object count.
     */
    size_t getSize() const;

    /**
     * @brief Checks if the worker thread is active.
     * @return True if the thread is running.
     */
    bool isRunning() const;

    /**
     * @brief Checks if the processor is ready to accept new objects.
     * @return True if ready.
     */
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
 * @brief Manages a dynamic pool of Processor worker threads.
 * @details Distributes ManagedObjects across processors and automatically
 * scales the number of workers up or down based on configured load thresholds.
 */
class ProcessorPool {
public:
    /**
     * @brief Default constructor. Creates pool with default
     * parameters and initializes the minimum number of processors.
     */
    ProcessorPool();

    /**
     * @brief Constructs the pool and initializes the minimum number of
     * processors.
     * @param maxProc Maximum number of worker threads.
     * @param minProc Minimum number of worker threads.
     * @param procCapacity Maximum objects per thread before scaling up.
     * @param maxLoad Load percentage threshold to trigger scaling up.
     * @param minLoad Load percentage threshold to trigger scaling down.
     * @param sleepInterval Sleep interval passed to worker threads.
     */
    ProcessorPool(size_t maxProc, size_t minProc, size_t procCapacity, types::loadt maxLoad,
                  types::loadt minLoad, types::ms sleepInterval);

    /**
     * @brief Move constructor.
     */
    ProcessorPool(ProcessorPool&& other);

    /**
     * @brief Move assignment operator.
     */
    ProcessorPool& operator=(ProcessorPool&& other);

    /**
     * @brief Destructor. Stops and joins all worker threads.
     */
    ~ProcessorPool();

    /**
     * @brief Registers an object with the pool, assigning it to the least
     * loaded processor.
     * @details May trigger the creation of a new processor if load thresholds
     * are exceeded.
     * @param obj Shared pointer to the managed object.
     * @return True if the object was successfully registered.
     */
    bool registerObject(std::shared_ptr<utils::ManagedObject> obj);

    /**
     * @brief Removes an object from the pool.
     * @details May trigger the removal of a processor if load drops below
     * thresholds.
     * @param obj Shared pointer to the managed object.
     * @return True if the object was found and removed.
     */
    bool removeObject(std::shared_ptr<utils::ManagedObject> obj);

    /**
     * @brief Removes an object from the pool by its ID.
     * @param id Unique identifier of the object.
     * @return True if the object was found and removed.
     */
    bool removeObject(types::ObjID id);

    /**
     * @brief Locates the processor managing the specified object.
     * @param obj Shared pointer to the managed object.
     * @return Shared pointer to the processor, or nullptr if not found.
     */
    std::shared_ptr<Processor> getProcessorContains(
        const std::shared_ptr<utils::ManagedObject> obj) const;

    /**
     * @brief Locates the processor managing the object with the specified ID.
     * @param id Unique identifier of the object.
     * @return Shared pointer to the processor, or nullptr if not found.
     */
    std::shared_ptr<Processor> getProcessorContains(types::ObjID id) const;

    /**
     * @brief Signals all processors in the pool to stop.
     */
    void stopAll();

    /**
     * @brief Returns the total number of objects managed by the pool.
     * @return Total object count.
     */
    size_t getObjCount() const;

    /**
     * @brief Returns the current number of active processors.
     * @return Active processor count.
     */
    size_t getProcCount() const;

protected:
    /** @brief Finds the processor containing the object without acquiring the
     * pool lock. */
    std::shared_ptr<Processor> getProcessorContains_nolock(types::ObjID id) const;

    /** @brief Evaluates pool load and scales processors up or down if
     * necessary. */
    bool adjustProcessors_nolock();

    /** @brief Instantiates and adds a new processor to the pool. */
    bool addProcessor_nolock();

    /** @brief Removes the least loaded processor and redistributes its objects.
     */
    bool removeProcessor_nolock();

    /** @brief Migrates all objects from the specified processor to other active
     * processors. */
    bool redistributeObjects_nolock(const std::shared_ptr<Processor>& processor);

    /** @brief Retrieves the least loaded processor that is currently ready. */
    std::shared_ptr<Processor> getLeastLoadProc_nolock() const;

    /** @brief Retrieves a processor by its index without lock validation. */
    std::shared_ptr<Processor> getProc_nolock(size_t id) const;

    /** @brief Finds the index of the least loaded ready processor. */
    size_t getLeastLoadProcId_nolock() const;

    /** @brief Returns a copy of the internal processor vector. */
    std::vector<std::shared_ptr<Processor>> getProcessors_nolock() const;

public:
    /**
     * @class Builder
     * @brief Builder pattern implementation for constructing a ProcessorPool.
     */
    class Builder {
    public:
        Builder() = default;

        /** @brief Sets the minimum number of processors. */
        Builder& setMinimumProcessors(size_t minProc);

        /** @brief Sets the maximum number of processors. */
        Builder& setMaximumProcessors(size_t maxProc);

        /** @brief Sets the maximum object capacity per processor. */
        Builder& setProcessorsCapacity(size_t procCapacity);

        /** @brief Sets the sleep interval for processor threads. */
        Builder& setSleepInterval(types::ms sleepInterval);

        /** @brief Sets the load percentage threshold for scaling up. */
        Builder& setMaximumLoad(types::loadt maxLoad);

        /** @brief Sets the load percentage threshold for scaling down. */
        Builder& setMinimumLoad(types::loadt minLoad);

        /**
         * @brief Constructs and returns the configured ProcessorPool.
         * @return Instantiated ProcessorPool.
         */
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