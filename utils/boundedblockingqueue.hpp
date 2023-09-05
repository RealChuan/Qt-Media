#pragma once

#include <QMutex>
#include <QWaitCondition>

#include <deque>

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

    void append(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_back(x);
        m_notEmpty.wakeOne();
    }

    void append(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_back(std::move(x));
        m_notEmpty.wakeOne();
    }

    void insertHead(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_front(x);
        m_notEmpty.wakeOne();
    }

    void insertHead(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_front(std::move(x));
        m_notEmpty.wakeOne();
    }

    auto take() -> T
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.empty()) {
            m_notEmpty.wait(&m_mutex);
        }
        T front(std::move(m_queue.front()));
        m_queue.pop_front();
        m_notFull.wakeOne();
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

    void setMaxSize(int maxSize)
    {
        QMutexLocker locker(&m_mutex);
        m_maxSize = maxSize;
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
    std::deque<T> m_queue;
    int m_maxSize;
};

} // namespace Utils
