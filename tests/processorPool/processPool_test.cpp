#include <gtest/gtest.h>
#include "processorPool.hpp"

using namespace ioteye;
class TestObject : public ManagedObject {
public:
    void process() override {
        processed = true;
    }
    bool processed = false;
};

class ProcessorPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = ProcessorPool::Builder()
            .setMinimumProcessors(1)
            .setMaximumProcessors(3)
            .setProcessorsCapacity(50)
            .setSleepInterval(ms(10))
            .setMaximumLoad(80)
            .setMinimumLoad(20)
            .build();
    }

    ProcessorPool pool;
};

TEST(ProcessorPoolTest, ManagedObjectBasic) {
    TestObject obj;
    EXPECT_NE(obj.getID(), 0);
    obj.process();
    EXPECT_TRUE(obj.processed);
}

TEST(ProcessorPoolTest, ProcessorBasicOperations) {
    auto processor = std::make_shared<Processor>(ms(10));
    auto obj = std::make_shared<TestObject>();
    
    EXPECT_TRUE(processor->addObject(obj));
    EXPECT_EQ(processor->getSize(), 1);
    EXPECT_TRUE(processor->contains(obj));
    EXPECT_TRUE(processor->contains(obj->getID()));
    
    processor->stop();
    EXPECT_FALSE(processor->isRunning());
}

TEST_F(ProcessorPoolTest, ObjectRegistration) {
    auto obj = std::make_shared<TestObject>();
    
    EXPECT_TRUE(pool.registerObject(obj));
    EXPECT_FALSE(pool.registerObject(obj));
    
    auto processor = pool.getProcessorContains(obj);
    ASSERT_NE(processor, nullptr);
    EXPECT_TRUE(processor->contains(obj));
}

TEST_F(ProcessorPoolTest, ObjectRemoval) {
    auto obj = std::make_shared<TestObject>();
    pool.registerObject(obj);
    
    EXPECT_TRUE(pool.removeObject(obj));
    EXPECT_FALSE(pool.removeObject(obj));
    
    EXPECT_EQ(pool.getProcessorContains(obj), nullptr);
}

TEST_F(ProcessorPoolTest, ProcessorScaling) {
    auto obj1 = std::make_shared<TestObject>();
    auto obj2 = std::make_shared<TestObject>();
    auto obj3 = std::make_shared<TestObject>();
    
    pool.registerObject(obj1);
    pool.registerObject(obj2);
    
    pool.registerObject(obj3);
    
    EXPECT_GE(pool.getProcessorContains(obj1)->getSize(), 1);
    EXPECT_GE(pool.getProcessorContains(obj3)->getSize(), 1);
}

TEST_F(ProcessorPoolTest, RedistributionOnRemove) {
    auto obj1 = std::make_shared<TestObject>();
    auto obj2 = std::make_shared<TestObject>();
    auto obj3 = std::make_shared<TestObject>();
    
    pool.registerObject(obj1);
    pool.registerObject(obj2);
    pool.registerObject(obj3);
    
    EXPECT_TRUE(pool.removeObject(obj1));
    EXPECT_TRUE(pool.removeObject(obj2));
}

TEST_F(ProcessorPoolTest, MultiThreadedAccess) {
    constexpr int NUM_OBJECTS = 100;
    std::vector<std::shared_ptr<TestObject>> objects;
    
    for (int i = 0; i < NUM_OBJECTS; ++i) {
        objects.push_back(std::make_shared<TestObject>());
    }
    
    std::vector<std::thread> threads;
    for (auto& obj : objects) {
        threads.emplace_back([&]() {
            EXPECT_TRUE(pool.registerObject(obj));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (auto& obj : objects) {
        EXPECT_NE(pool.getProcessorContains(obj), nullptr);
    }
}

TEST_F(ProcessorPoolTest, StopAllProcessors) {
    auto obj = std::make_shared<TestObject>();
    pool.registerObject(obj);
    
    pool.stopAll();
    
    auto processor = pool.getProcessorContains(obj);
    ASSERT_NE(processor, nullptr);
    EXPECT_FALSE(processor->isRunning());
}

TEST(ProcessorPoolTest, BuilderConfiguration) {
    auto customPool = ProcessorPool::Builder()
        .setMinimumProcessors(2)
        .setMaximumProcessors(5)
        .setProcessorsCapacity(10)
        .setSleepInterval(ms(50))
        .setMaximumLoad(90)
        .setMinimumLoad(10)
        .build();
    
    EXPECT_EQ(customPool.getProcessorContains(999), nullptr);
}

TEST(ProcessorPoolTest, MoveSemantics) {
    ProcessorPool sourcePool = ProcessorPool::Builder()
        .setMinimumProcessors(1)
        .setProcessorsCapacity(1)
        .build();
    
    auto obj = std::make_shared<TestObject>();
    ASSERT_TRUE(sourcePool.registerObject(obj));
    
    ProcessorPool destPool = std::move(sourcePool);
    
    EXPECT_EQ(sourcePool.getProcessorContains(obj), nullptr);
    EXPECT_NE(destPool.getProcessorContains(obj), nullptr);
    
    auto newObj = std::make_shared<TestObject>();
    EXPECT_TRUE(destPool.registerObject(newObj));
}

TEST_F(ProcessorPoolTest, ObjectProcessing) {
    auto obj = std::make_shared<TestObject>();
    pool.registerObject(obj);
    
    std::this_thread::sleep_for(ms(50));
    
    EXPECT_TRUE(obj->processed);
}

TEST(ProcessorPoolTest, CapacityLimits) {
    auto smallPool = ProcessorPool::Builder()
        .setProcessorsCapacity(1)
        .setMaximumProcessors(1)
        .build();
    
    auto obj1 = std::make_shared<TestObject>();
    auto obj2 = std::make_shared<TestObject>();
    
    EXPECT_TRUE(smallPool.registerObject(obj1));
    EXPECT_FALSE(smallPool.registerObject(obj2));
}