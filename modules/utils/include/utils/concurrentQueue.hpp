#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace ioteye::utils {

template <typename T>
class ConcurrentQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        m_cv.notify_one();
    }

    bool pop(T& item, std::chrono::milliseconds timeout =
                          std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return !m_queue.empty(); }))
            return false;
        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
};

}  // namespace ioteye::utils