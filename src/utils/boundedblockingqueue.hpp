#pragma once

#include <QMutex>
#include <QWaitCondition>

#include <deque>

namespace Utils {

template<typename T, typename = void>
struct is_smart_or_non_pointer
{
    static constexpr bool value = false;
};

template<typename T>
struct is_smart_or_non_pointer<
    T,
    std::enable_if_t<std::is_base_of<std::unique_ptr<typename std::decay<T>::type>, T>::value
                     || std::is_base_of<std::shared_ptr<typename std::decay<T>::type>, T>::value
                     || !std::is_pointer<T>::value>>
{
    static constexpr bool value = true;
};

template<typename T>
class BoundedBlockingQueue
{
    static_assert(is_smart_or_non_pointer<T>::value,
                  "T must be a smart pointer or a non-pointer type.");

    Q_DISABLE_COPY_MOVE(BoundedBlockingQueue);

    mutable QMutex m_mutex;
    QWaitCondition m_notEmpty;
    QWaitCondition m_notFull;
    std::deque<T> m_queue;
    std::atomic_int m_maxSize;

public:
    explicit BoundedBlockingQueue(int maxSize)
        : m_maxSize(maxSize)
    {}

    void append(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize.load()) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_back(x);
        m_notEmpty.wakeOne();
    }

    void append(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize.load()) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_back(std::move(x));
        m_notEmpty.wakeOne();
    }

    void insertHead(const T &x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize.load()) {
            m_notFull.wait(&m_mutex);
        }
        m_queue.push_front(x);
        m_notEmpty.wakeOne();
    }

    void insertHead(T &&x)
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.size() >= m_maxSize.load()) {
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

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_queue.clear();
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
        return m_queue.size() >= m_maxSize.load();
    }

    [[nodiscard]] auto size() const -> size_t
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size();
    }

    void setMaxSize(int maxSize) { m_maxSize.store(maxSize); }

    [[nodiscard]] auto maxSize() const -> int { return m_maxSize.load(); }
};

} // namespace Utils
