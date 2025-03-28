#pragma once

#include <QMutexLocker>
#include <QObject>
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

    Q_DISABLE_COPY_MOVE(BoundedBlockingQueue)

    std::deque<T> m_queue;
    mutable QMutex m_mutex;
    QWaitCondition m_notEmpty;
    QWaitCondition m_notFull;
    std::atomic<std::size_t> m_maxSize{1000};
    std::atomic_bool m_abort = false;

public:
    BoundedBlockingQueue() = default;
    explicit BoundedBlockingQueue(std::size_t max_size)
        : m_maxSize(max_size)
    {}
    ~BoundedBlockingQueue()
    {
        abort();
        clear();
    }

    void push_back(const T &value)
    {
        {
            QMutexLocker locker(&m_mutex);
            while (!m_abort.load() && m_queue.size() >= m_maxSize.load()) {
                m_notFull.wait(&m_mutex);
            }
            if (m_abort.load()) {
                return;
            }
            m_queue.push_back(value);
        }
        m_notEmpty.wakeOne();
    }

    void push_back(T &&value)
    {
        {
            QMutexLocker locker(&m_mutex);
            while (!m_abort.load() && m_queue.size() >= m_maxSize.load()) {
                m_notFull.wait(&m_mutex);
            }
            if (m_abort.load()) {
                return;
            }
            m_queue.push_back(std::move(value));
        }
        m_notEmpty.wakeOne();
    }

    void push_front(const T &value)
    {
        {
            QMutexLocker locker(&m_mutex);
            while (!m_abort.load() && m_queue.size() >= m_maxSize.load()) {
                m_notFull.wait(&m_mutex);
            }
            if (m_abort.load()) {
                return;
            }
            m_queue.push_front(value);
        }
        m_notEmpty.wakeOne();
    }

    void push_front(T &&value)
    {
        {
            QMutexLocker locker(&m_mutex);
            while (!m_abort.load() && m_queue.size() >= m_maxSize.load()) {
                m_notFull.wait(&m_mutex);
            }
            if (m_abort.load()) {
                return;
            }
            m_queue.push_front(std::move(value));
        }
        m_notEmpty.wakeOne();
    }

    void push(const std::deque<T> &values)
    {
        {
            QMutexLocker locker(&m_mutex);
            m_queue.insert(m_queue.end(), values.begin(), values.end());
        }
        m_notEmpty.wakeAll();
    }

    void push(std::deque<T> &&values)
    {
        {
            QMutexLocker locker(&m_mutex);
            m_queue.insert(m_queue.end(),
                           std::make_move_iterator(values.begin()),
                           std::make_move_iterator(values.end()));
        }
        m_notEmpty.wakeAll();
    }

    T take()
    {
        QMutexLocker locker(&m_mutex);
        while (m_queue.empty() && !m_abort.load()) {
            m_notEmpty.wait(&m_mutex);
        }
        if (m_abort.load()) {
            return T();
        }
        T value = std::move(m_queue.front());
        m_queue.pop_front();
        m_notFull.wakeOne();
        return value;
    }

    void unique()
    {
        {
            QMutexLocker locker(&m_mutex);
            if (m_queue.empty()) {
                return;
            }
            std::sort(m_queue.begin(), m_queue.end(), [](const T &a, const T &b) { return a > b; });
            auto last = std::unique(m_queue.begin(), m_queue.end());
            m_queue.erase(last, m_queue.end());
        }
        m_notFull.wakeAll();
    }

    void clear()
    {
        {
            QMutexLocker locker(&m_mutex);
            m_queue.clear();
        }
        m_notFull.wakeAll();
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

    void setMaxSize(std::size_t max_size) { m_maxSize.store(max_size); }

    std::size_t maxSize() const { return m_maxSize.load(); }

    bool isFull() const
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.size() >= m_maxSize.load();
    }

    void start() { m_abort.store(false); }

    void abort()
    {
        m_abort.store(true);
        m_notEmpty.wakeAll();
        m_notFull.wakeAll();
    }

    bool isAborted() const { return m_abort.load(); }
};

} // namespace Utils
