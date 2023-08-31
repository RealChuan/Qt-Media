#pragma once

#include <QMutex>
#include <queue>

namespace Utils {

template<typename T>
class ThreadSafeQueue
{
    Q_DISABLE_COPY_MOVE(ThreadSafeQueue);

public:
    using ClearCallback = std::function<void(T &)>;

    explicit ThreadSafeQueue() = default;

    void put(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push(x);
    }

    void put(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        m_queue.push(std::move(x));
    }

    void putHead(const T &x)
    {
        std::queue<T> temp;
        temp.push(x);
        QMutexLocker locker(&m_mutex);
        while (!m_queue.empty()) {
            temp.push(m_queue.front());
            m_queue.pop();
        }
        m_queue.swap(temp);
    }

    void putHead(T &&x)
    {
        std::queue<T> temp;
        temp.push(x);
        QMutexLocker locker(&m_mutex);
        while (!m_queue.empty()) {
            temp.push(m_queue.front());
            m_queue.pop();
        }
        m_queue.swap(temp);
    }

    auto take() -> T
    {
        QMutexLocker locker(&m_mutex);
        if (m_queue.empty()) {
            return T();
        }
        T front(std::move(m_queue.front()));
        m_queue.pop();
        return front;
    }

    void clear(ClearCallback callback = nullptr)
    {
        QMutexLocker locker(&m_mutex);
        while (!m_queue.empty()) {
            if (callback != nullptr) {
                callback(m_queue.front());
            }
            m_queue.pop();
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
    std::queue<T> m_queue;
};

} // namespace Utils
