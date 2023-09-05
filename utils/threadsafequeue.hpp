#pragma once

#include <QMutex>

#include <deque>
#include <functional>

namespace Utils {

template<typename T>
class ThreadSafeQueue
{
    Q_DISABLE_COPY_MOVE(ThreadSafeQueue);

public:
    using ClearCallback = std::function<void(T &)>;

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

    void clear(ClearCallback callback = nullptr)
    {
        QMutexLocker locker(&m_mutex);
        if (callback) {
            while (!m_queue.empty()) {
                if (callback) {
                    callback(m_queue.front());
                }
                m_queue.pop_front();
            }
        } else if (!m_queue.empty()) {
            m_queue.clear();
        }
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

private:
    mutable QMutex m_mutex;
    std::deque<T> m_queue;
};

} // namespace Utils
