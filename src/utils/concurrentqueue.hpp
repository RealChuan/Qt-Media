#pragma once

#include "boundedblockingqueue.hpp"

namespace Utils {

template<typename T>
class ConcurrentQueue
{
    static_assert(is_smart_or_non_pointer<T>::value,
                  "T must be a smart pointer or a non-pointer type.");

    Q_DISABLE_COPY_MOVE(ConcurrentQueue)

    std::deque<T> m_queue;
    mutable QMutex m_mutex;

public:
    explicit ConcurrentQueue() = default;

    void push_back(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_back(x);
    }

    void push_back(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_back(std::move(x));
    }

    void push_front(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push_front(x);
    }

    void push_front(T &&x)
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

    bool isEmpty() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }

    int size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }
};

} // namespace Utils
