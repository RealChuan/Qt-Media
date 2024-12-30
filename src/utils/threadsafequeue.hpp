#pragma once

#include "boundedblockingqueue.hpp"

namespace Utils {

template<typename T>
class ThreadSafeQueue
{
    static_assert(is_smart_or_non_pointer<T>::value,
                  "T must be a smart pointer or a non-pointer type.");

    Q_DISABLE_COPY_MOVE(ThreadSafeQueue);

    mutable QMutex m_mutex;
    std::deque<T> m_queue;

public:
    explicit ThreadSafeQueue() = default;

    void append(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_back(x);
    }

    void append(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_back(std::move(x));
    }

    void insertHead(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_front(x);
    }

    void insertHead(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_front(std::move(x));
    }

    auto take() -> T
    {
        QMutexLocker locker(&m_mutex);
        if (m_queue.empty()) {
            return T();
        }
        T front(std::move(m_queue.front()));
        m_queue.pop_front();
        return front;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
    }

    [[nodiscard]] auto size() const -> int
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

    [[nodiscard]] auto empty() const -> bool
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }
};

} // namespace Utils
