#pragma once

#include <QMutex>
#include <QWaitCondition>
#include <queue>

namespace Utils {

template<typename T>
class BoundedBlockingQueue
{
    Q_DISABLE_COPY_MOVE(BoundedBlockingQueue);

public:
    using ClearCallback = std::function<void(T &)>;

    explicit BoundedBlockingQueue(int maxSize)
        : m_maxSize(maxSize)
    {}

    void put(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push(x);
        m_notEmpty.wakeOne();
    }

    void put(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push(std::move(x));
        m_notEmpty.wakeOne();
    }

    auto take() -> T
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.empty()) {
            m_notEmpty.wait(&m_mutex);
        }
        T front(std::move(m_queue.front()));
        m_queue.pop();
        m_notFull.wakeOne();
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
        m_notFull.wakeAll();
    }

    [[nodiscard]] auto empty() const -> bool
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }

    [[nodiscard]] auto full() const -> bool
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size() >= m_maxSize;
    }

    [[nodiscard]] size_t size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

    [[nodiscard]] size_t maxSize() const
    {
        QMutexLocker locker(&m_mutex);
        return m_maxSize;
    }

private:
    mutable QMutex m_mutex;
    QWaitCondition m_notEmpty;
    QWaitCondition m_notFull;
    std::queue<T> m_queue;
    int m_maxSize;
};

} // namespace Utils
