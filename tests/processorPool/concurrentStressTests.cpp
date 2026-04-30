#include <gtest/gtest.h>
#include <atomic>
#include <future>
#include <memory>
#include <random>

#include <processorPool.hpp>

using namespace ioteye;

// Тестовый класс, наследник ManagedObject
class TestObject : public ManagedObject {
public:
    std::atomic<int> processCount{0};
    
    void process() override {
        processCount++;
    }
};

class TestableProcessorPool : public ProcessorPool {
public:
    // Делаем protected методы публичными для тестирования
    using ProcessorPool::adjustProcessors;
    using ProcessorPool::addProcessor;
    using ProcessorPool::removeProcessor;
    using ProcessorPool::redistributeObjects;
    using ProcessorPool::getLeastLoadProc;
    using ProcessorPool::getLeastLoadProcId;
    using ProcessorPool::getProcessors;

    // Наследуем конструкторы
    using ProcessorPool::ProcessorPool;
};

// Тест на потокобезопасность ManagedObject
TEST(ThreadSafetyTests, ManagedObjectThreadSafety) {
    const int THREAD_COUNT = 100;
    const int OPERATIONS_PER_THREAD = 1000;
    
    auto obj = std::make_shared<TestObject>();
    std::vector<std::thread> threads;
    
    // Запускаем потоки, которые будут вызывать методы объекта
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&obj, OPERATIONS_PER_THREAD]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                // Получаем ID (использует shared_mutex)
                auto id = obj->getID();
                EXPECT_EQ(id, 1);
                // Вызываем process (должен быть потокобезопасным)
                obj->process();
            }
        });
    }
    
    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }
    
    // Проверяем, что все вызовы process были выполнены
    EXPECT_EQ(obj->processCount, THREAD_COUNT * OPERATIONS_PER_THREAD);
}

// Тест на потокобезопасность Processor
TEST(ThreadSafetyTests, ProcessorThreadSafety) {
    const int THREAD_COUNT = 50;
    const int OBJECT_COUNT = 100;
    const int OPERATIONS_PER_THREAD = 1000;
    
    Processor processor(ms(1));
    std::vector<std::shared_ptr<TestObject>> objects;
    
    // Создаем тестовые объекты
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        auto obj = std::make_shared<TestObject>();
        objects.push_back(obj);
        processor.addObject(obj);
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> adds(0), removes(0), moves(0);
    
    // Запускаем потоки, которые будут выполнять операции с процессором
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&processor, &objects, OPERATIONS_PER_THREAD, &adds, &removes, &moves]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> opDist(0, 2);
            std::uniform_int_distribution<> objDist(0, objects.size() - 1);
            
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int op = opDist(gen);
                auto obj = objects[objDist(gen)];
                
                switch (op) {
                    case 0: // add
                        if (processor.addObject(obj)) adds++;
                        break;
                    case 1: // remove
                        if (processor.removeObject(obj)) removes++;
                        break;
                    case 2: // contains
                        processor.contains(obj);
                        break;
                }
            }
        });
    }
    
    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }
    
    // Проверяем, что процессор все еще работает
    EXPECT_TRUE(processor.isRunning());
    
    // Проверяем, что размер контейнера объектов корректен
    size_t expectedSize = OBJECT_COUNT + adds - removes;
    EXPECT_EQ(processor.getSize(), expectedSize);
}

// Тест на потокобезопасность ProcessorPool
TEST(ThreadSafetyTests, ProcessorPoolThreadSafety) {
    const int THREAD_COUNT = 50;
    const int OBJECT_COUNT = 1000;
    const int OPERATIONS_PER_THREAD = 500;
    
    TestableProcessorPool pool(2, 10, 100, 80, 20, ms(1));
    std::vector<std::shared_ptr<TestObject>> objects;
    
    // Создаем тестовые объекты
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        objects.push_back(std::make_shared<TestObject>());
    }
    
    std::vector<std::thread> threads;
    std::atomic<int> adds(0), removes(0);
    
    // Запускаем потоки, которые будут выполнять операции с пулом
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&pool, &objects, OPERATIONS_PER_THREAD, &adds, &removes]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> opDist(0, 1);
            std::uniform_int_distribution<> objDist(0, objects.size() - 1);
            
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int op = opDist(gen);
                auto obj = objects[objDist(gen)];
                
                switch (op) {
                    case 0: // register
                        if (pool.registerObject(obj)) adds++;
                        break;
                    case 1: // remove
                        if (pool.removeObject(obj)) removes++;
                        break;
                }
            }
        });
    }
    
    // Ждем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }
    
    // Проверяем, что количество объектов в пуле корректно
    size_t expectedObjects = adds - removes;
    
    // Подсчитываем общее количество объектов в пуле
    size_t actualObjects = 0;
    auto processors = pool.getProcessors();
    for (const auto& proc : processors) {
        actualObjects += proc->getSize();
    }
    
    EXPECT_EQ(actualObjects, expectedObjects);
    
    // Проверяем, что количество процессоров в допустимых пределах
    EXPECT_GE(processors.size(), 2u);
    EXPECT_LE(processors.size(), 10u);
}

// Тест на корректность работы процессоров под нагрузкой
TEST(ThreadSafetyTests, ProcessorPoolUnderLoad) {
    const int OBJECT_COUNT = 500;
    const int TEST_DURATION_MS = 5000;
    
    TestableProcessorPool pool(2, 8, 100, 70, 30, ms(1));
    std::vector<std::shared_ptr<TestObject>> objects;
    
    // Создаем и регистрируем объекты
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        auto obj = std::make_shared<TestObject>();
        objects.push_back(obj);
        EXPECT_TRUE(pool.registerObject(obj));
    }
    
    // Запускаем тест на определенное время
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < TEST_DURATION_MS) {
        
        // Периодически добавляем/удаляем объекты
        static int counter = 0;
        if (counter++ % 100 == 0) {
            auto obj = std::make_shared<TestObject>();
            pool.registerObject(obj);
            objects.push_back(obj);
            
            if (!objects.empty()) {
                pool.removeObject(objects.back());
                objects.pop_back();
            }
        }
    }
    
    // Проверяем, что все процессоры все еще работают
    auto processors = pool.getProcessors();
    for (const auto& proc : processors) {
        EXPECT_TRUE(proc->isRunning());
    }
    
    // Проверяем, что все объекты были обработаны хотя бы раз
    for (const auto& obj : objects) {
        EXPECT_GT(obj->processCount.load(), 0);
    }
    
    // Останавливаем пул
    pool.stopAll();
    
    // Проверяем, что все процессоры остановились
    for (const auto& proc : processors) {
        EXPECT_FALSE(proc->isRunning());
    }
}