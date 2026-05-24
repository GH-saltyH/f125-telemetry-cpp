#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

// 스레드 안전 큐 (Thread-Safe Queue)
template <typename T>
class SafeQueue {
private:
    std::queue<T> m_queue;
    std::mutex    m_mutex;
    std::condition_variable m_cv;

public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(value));
        }
        m_cv.notify_one();
    }

    // timeout == 0ms 이면 즉시 반환 (non-blocking pop)
    bool pop(T& value, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_cv.wait_for(lock, timeout, [this] { return !m_queue.empty(); }))
            return false;
        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
};