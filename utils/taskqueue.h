#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <QMutex>

namespace Utils {

/*----------------------------------------------------------------------------/
/                            线程安全的有界队列 模板                           /
/----------------------------------------------------------------------------*/

template<typename T>
class Queue
{
public:
    Queue()
        : m_T()
        , m_mutex()
        , m_maxSize(100)
    {}
    ~Queue(){}

    bool append(const T& t)
    {
        QMutexLocker lock(&m_mutex);
        if(m_T.size() >= m_maxSize)
            return false;
        m_T.append(std::move(t));
        return true;
    }

    bool append(T&& t)
    {
        QMutexLocker lock(&m_mutex);
        if(m_T.size() >= m_maxSize)
            return false;
        m_T.append(std::move(t));
        return true;
    }

    T takeFirst()
    {
        QMutexLocker lock(&m_mutex);
        if(!m_T.isEmpty())
            return m_T.takeFirst();
        return T();
    }

    bool isEmpty()
    {
        QMutexLocker lock(&m_mutex);
        if(m_T.isEmpty())
            return true;
        return false;
    }

    void clear()
    {
        QMutexLocker lock(&m_mutex);
        m_T.clear();
    }

    int size() const
    {
        QMutexLocker lock(&m_mutex);
        return m_T.size();
    }

    void setMaxSize(const int maxSize)
    {
        QMutexLocker lock(&m_mutex);
        m_maxSize = maxSize;
    }

private:
    QQueue<T> m_T;
    mutable QMutex m_mutex;
    int m_maxSize;
};

}

#endif // TASKQUEUE_H
