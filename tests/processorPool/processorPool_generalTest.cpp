#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <common/types.hpp>
#include <memory>
#include <random>
#include <thread>
#include <vector>

// Testable
#include <utils/processorPool.hpp>

namespace test_utils {

// Polls a predicate until it returns true or the timeout is exceeded.
// Eliminates flakiness caused by arbitrary sleep_for() calls in CI
// environments.
template <typename Predicate>
bool waitFor(Predicate pred, std::chrono::milliseconds timeout = std::chrono::seconds(2)) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > timeout) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

}  // namespace test_utils

namespace {

using namespace ioteye::utils;
using namespace ioteye::types;

class TestManagedObject : public ManagedObject {
public:
    explicit TestManagedObject(ObjID id) {
        m_id = id;
    }

    TestManagedObject() : ManagedObject::ManagedObject() {
    }

    void process() override {
        m_processCount.fetch_add(1, std::memory_order_relaxed);
    }

    int getProcessCount() const {
        return m_processCount.load(std::memory_order_relaxed);
    }

private:
    std::atomic<int> m_processCount{0};
};

class ProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_proc = std::make_shared<Processor>(ms(1));
    }

    void TearDown() override {
        if (m_proc) {
            m_proc->stop();
        }
    }

    std::shared_ptr<Processor> m_proc;
};

TEST_F(ProcessorTest, AddAndContains) {
    auto obj = std::make_shared<TestManagedObject>(101);

    EXPECT_TRUE(m_proc->addObject(obj));
    EXPECT_FALSE(m_proc->addObject(obj));  // Duplicate

    EXPECT_TRUE(m_proc->contains(obj));
    EXPECT_TRUE(m_proc->contains(101));
    EXPECT_EQ(m_proc->getSize(), 1);
}

TEST_F(ProcessorTest, RemoveObject) {
    auto obj = std::make_shared<TestManagedObject>(102);
    m_proc->addObject(obj);

    EXPECT_TRUE(m_proc->removeObject(102));
    EXPECT_FALSE(m_proc->contains(102));
    EXPECT_EQ(m_proc->getSize(), 0);

    EXPECT_FALSE(m_proc->removeObject(102));  // Already removed
}

TEST_F(ProcessorTest, MoveObjectBetweenProcessors) {
    auto proc2 = std::make_shared<Processor>(ms(1));
    auto obj = std::make_shared<TestManagedObject>(103);

    m_proc->addObject(obj);
    EXPECT_TRUE(m_proc->contains(103));

    EXPECT_TRUE(m_proc->moveObject(proc2, 103));

    EXPECT_FALSE(m_proc->contains(103));
    EXPECT_TRUE(proc2->contains(103));
    EXPECT_EQ(proc2->getSize(), 1);

    proc2->stop();
}

TEST_F(ProcessorTest, ProcessingLoopExecution) {
    auto obj = std::make_shared<TestManagedObject>(104);
    m_proc->addObject(obj);

    // Replaces flaky std::this_thread::sleep_for(15ms)
    bool processed = test_utils::waitFor([&]() { return obj->getProcessCount() > 0; });

    EXPECT_TRUE(processed) << "Worker thread failed to process object within timeout";
}

TEST_F(ProcessorTest, ConcurrentAddRemove) {
    std::vector<std::thread> threads;
    std::atomic<bool> stopFlag{false};

    // Hammer addObject from multiple threads
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, i, &stopFlag]() {
            ObjID baseId = i * 1000;
            while (!stopFlag) {
                for (int j = 0; j < 10; ++j) {
                    auto obj = std::make_shared<TestManagedObject>(baseId + j);
                    m_proc->addObject(obj);
                    m_proc->removeObject(baseId + j);
                }
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stopFlag = true;

    for (auto& t : threads) {
        t.join();
    }

    // State should be consistent, no crashes or deadlocks
    EXPECT_GE(m_proc->getSize(), 0);
}

TEST_F(ProcessorTest, ConcurrentMoveObjectNoDeadlock) {
    auto proc1 = std::make_shared<Processor>(ms(1));
    auto proc2 = std::make_shared<Processor>(ms(1));

    for (int i = 0; i < 100; ++i) {
        proc1->addObject(std::make_shared<TestManagedObject>(i));
        proc2->addObject(std::make_shared<TestManagedObject>(1000 + i));
    }

    std::atomic<bool> stop{false};

    // Hammers moveObject in both directions simultaneously.
    // If lock ordering is not strictly enforced (e.g., by memory address),
    // this will deadlock and the test will hang/timeout.
    std::thread t1([&]() {
        while (!stop) {
            for (int i = 0; i < 100; ++i)
                proc1->moveObject(proc2, i);
        }
    });

    std::thread t2([&]() {
        while (!stop) {
            for (int i = 0; i < 100; ++i)
                proc2->moveObject(proc1, 1000 + i);
        }
    });

    // Allow threads to attempt multiple cross-migrations
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop = true;

    t1.join();
    t2.join();

    // If execution reaches here, no deadlock occurred.
    SUCCEED();
}

class ProcessorPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_pool = std::make_shared<ProcessorPool>(4,     // maxProc
                                                 1,     // minProc
                                                 10,    // procCapacity
                                                 80.0,  // maxLoad
                                                 20.0,  // minLoad
                                                 ms(1)  // sleepInterval
        );
    }

    void TearDown() override {
        if (m_pool) {
            m_pool->stopAll();
        }
    }

    std::shared_ptr<ProcessorPool> m_pool;
};

TEST_F(ProcessorPoolTest, RegisterAndLocateObject) {
    auto obj = std::make_shared<TestManagedObject>(201);

    EXPECT_TRUE(m_pool->registerObject(obj));
    EXPECT_EQ(m_pool->getObjCount(), 1);

    auto proc = m_pool->getProcessorContains(201);
    EXPECT_NE(proc, nullptr);
    EXPECT_TRUE(proc->contains(201));
}

TEST_F(ProcessorPoolTest, ScaleUpOnHighLoad) {
    EXPECT_EQ(m_pool->getProcCount(), 1);

    // Add enough objects to exceed procCapacity (10) and trigger maxLoad (80%)
    // Assuming load = (objCount / (procCount * procCapacity)) * 100
    // 9 objects = 90% load on 1 processor (capacity 10)
    std::vector<std::shared_ptr<TestManagedObject>> objects;
    for (int i = 0; i < 15; ++i) {
        auto obj = std::make_shared<TestManagedObject>(300 + i);
        objects.push_back(obj);
        m_pool->registerObject(obj);
    }

    // Pool should have scaled up to at least 2 processors
    EXPECT_GE(m_pool->getProcCount(), 2);
    EXPECT_EQ(m_pool->getObjCount(), 15);
}

TEST_F(ProcessorPoolTest, ScaleDownAndRedistribute) {
    // Force scale up first
    std::vector<std::shared_ptr<TestManagedObject>> objects;
    for (int i = 0; i < 25; ++i) {
        auto obj = std::make_shared<TestManagedObject>(400 + i);
        objects.push_back(obj);
        m_pool->registerObject(obj);
    }

    size_t peakProcCount = m_pool->getProcCount();
    EXPECT_GE(peakProcCount, 2);

    // Remove objects to drop below minLoad (20%)
    // 2 objects on 3 processors (capacity 10) = ~6% load
    for (size_t i = 2; i < objects.size(); ++i) {
        m_pool->removeObject(objects[i]);
    }

    // Pool should scale down and redistribute remaining objects
    EXPECT_LT(m_pool->getProcCount(), peakProcCount);
    EXPECT_EQ(m_pool->getObjCount(), 2);

    // Verify remaining objects are still accessible
    EXPECT_NE(m_pool->getProcessorContains(400), nullptr);
    EXPECT_NE(m_pool->getProcessorContains(401), nullptr);
}

TEST_F(ProcessorPoolTest, RemoveNonExistentObject) {
    auto obj = std::make_shared<TestManagedObject>(501);
    EXPECT_FALSE(m_pool->removeObject(obj));
    EXPECT_FALSE(m_pool->removeObject(999));
    EXPECT_EQ(m_pool->getObjCount(), 0);
}

class TestableProcessorPool : public ProcessorPool {
public:
    using ProcessorPool::ProcessorPool;

    // Expose protected _nolock methods for white-box testing
    using ProcessorPool::addProcessor_nolock;
    using ProcessorPool::adjustProcessors_nolock;
    using ProcessorPool::getLeastLoadProc_nolock;
    using ProcessorPool::getLeastLoadProcId_nolock;
    using ProcessorPool::getProcessors_nolock;
    using ProcessorPool::redistributeObjects_nolock;
    using ProcessorPool::removeProcessor_nolock;
};

TEST(ThreadSafetyTests, ManagedObjectThreadSafety) {
    const int THREAD_COUNT = 100;
    const int OPERATIONS_PER_THREAD = 1000;

    auto obj = std::make_shared<TestManagedObject>();
    std::vector<std::thread> threads;

    // Capture the dynamically generated ID instead of assuming it is 1
    auto expectedId = obj->getObjID();

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&obj, expectedId, OPERATIONS_PER_THREAD]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                auto id = obj->getObjID();
                EXPECT_EQ(id, expectedId);
                obj->process();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(obj->getProcessCount(), THREAD_COUNT * OPERATIONS_PER_THREAD);
}

TEST(ThreadSafetyTests, ProcessorThreadSafety) {
    const int THREAD_COUNT = 50;
    const int OBJECT_COUNT = 100;
    const int OPERATIONS_PER_THREAD = 1000;

    Processor processor(std::chrono::milliseconds(1));
    std::vector<std::shared_ptr<TestManagedObject>> objects;

    for (int i = 0; i < OBJECT_COUNT; ++i) {
        auto obj = std::make_shared<TestManagedObject>();
        objects.push_back(obj);
        processor.addObject(obj);
    }

    std::vector<std::thread> threads;
    std::atomic<int> adds(0), removes(0);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&processor, &objects, OPERATIONS_PER_THREAD, &adds, &removes]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> opDist(0, 2);
            std::uniform_int_distribution<> objDist(0, static_cast<int>(objects.size()) - 1);

            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int op = opDist(gen);
                auto obj = objects[objDist(gen)];

                switch (op) {
                    case 0:
                        if (processor.addObject(obj))
                            adds.fetch_add(1, std::memory_order_relaxed);
                        break;
                    case 1:
                        if (processor.removeObject(obj))
                            removes.fetch_add(1, std::memory_order_relaxed);
                        break;
                    case 2:
                        processor.contains(obj);
                        break;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_TRUE(processor.isRunning());

    size_t expectedSize = OBJECT_COUNT + adds.load() - removes.load();
    EXPECT_EQ(processor.getSize(), expectedSize);

    processor.stop();
}

TEST(ThreadSafetyTests, ProcessorPoolThreadSafety) {
    const int THREAD_COUNT = 50;
    const int OBJECT_COUNT = 1000;
    const int OPERATIONS_PER_THREAD = 500;

    // Arguments: maxProc, minProc, procCapacity, maxLoad, minLoad,
    // sleepInterval maxProc must be >= minProc
    TestableProcessorPool pool(10, 2, 100, 80, 20, std::chrono::milliseconds(1));
    std::vector<std::shared_ptr<TestManagedObject>> objects;

    for (int i = 0; i < OBJECT_COUNT; ++i) {
        objects.push_back(std::make_shared<TestManagedObject>());
    }

    std::vector<std::thread> threads;
    std::atomic<int> adds(0), removes(0);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&pool, &objects, OPERATIONS_PER_THREAD, &adds, &removes]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> opDist(0, 1);
            std::uniform_int_distribution<> objDist(0, static_cast<int>(objects.size()) - 1);

            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int op = opDist(gen);
                auto obj = objects[objDist(gen)];

                switch (op) {
                    case 0:
                        if (pool.registerObject(obj))
                            adds.fetch_add(1, std::memory_order_relaxed);
                        break;
                    case 1:
                        if (pool.removeObject(obj))
                            removes.fetch_add(1, std::memory_order_relaxed);
                        break;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    size_t expectedObjects = adds.load() - removes.load();
    size_t actualObjects = 0;

    // Threads are joined, safe to call _nolock without holding m_poolMutex
    auto processors = pool.getProcessors_nolock();
    for (const auto& proc : processors) {
        actualObjects += proc->getSize();
    }

    EXPECT_EQ(actualObjects, expectedObjects);
    EXPECT_GE(processors.size(), 2u);
    EXPECT_LE(processors.size(), 10u);

    pool.stopAll();
}

TEST(ThreadSafetyTests, ProcessorPoolUnderLoad) {
    const int OBJECT_COUNT = 500;
    const int TEST_DURATION_MS = 2000;

    TestableProcessorPool pool(8, 2, 100, 70, 30, std::chrono::milliseconds(1));
    std::vector<std::shared_ptr<TestManagedObject>> objects;

    for (int i = 0; i < OBJECT_COUNT; ++i) {
        auto obj = std::make_shared<TestManagedObject>();
        objects.push_back(obj);
        EXPECT_TRUE(pool.registerObject(obj));
    }

    auto end_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(TEST_DURATION_MS);
    int counter = 0;

    while (std::chrono::steady_clock::now() < end_time) {
        if (counter++ % 100 == 0) {
            auto obj = std::make_shared<TestManagedObject>();
            if (pool.registerObject(obj)) {
                objects.push_back(obj);
            }

            if (objects.size() > static_cast<size_t>(OBJECT_COUNT)) {
                auto backObj = objects.back();
                if (pool.removeObject(backObj)) {
                    objects.pop_back();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    auto processors = pool.getProcessors_nolock();
    for (const auto& proc : processors) {
        EXPECT_TRUE(proc->isRunning());
    }

    int processedCount = 0;
    for (const auto& obj : objects) {
        if (obj->getProcessCount() > 0)
            processedCount++;
    }
    EXPECT_GT(processedCount, 0);

    pool.stopAll();

    // Allow background threads to observe m_running = false and exit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    processors = pool.getProcessors_nolock();
    for (const auto& proc : processors) {
        EXPECT_FALSE(proc->isRunning());
    }
}

}  // namespace