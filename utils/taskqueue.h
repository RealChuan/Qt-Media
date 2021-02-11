#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "utils_global.h"

#include <QMutex>
#include <QQueue>

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
        QMutexLocker locker(&m_mutex);
        if(m_T.size() >= m_maxSize)
            return false;
        m_T.append(std::move(t));
        return true;
    }

    bool append(T&& t)
    {
        QMutexLocker locker(&m_mutex);
        if(m_T.size() >= m_maxSize)
            return false;
        m_T.append(std::move(t));
        return true;
    }

    T takeFirst()
    {
        QMutexLocker locker(&m_mutex);
        if(!m_T.isEmpty())
            return m_T.takeFirst();
        return T();
    }

    bool isEmpty()
    {
        QMutexLocker locker(&m_mutex);
        if(m_T.isEmpty())
            return true;
        return false;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_T.clear();
    }

    int size() const
    {
        QMutexLocker locker(&m_mutex);
        return m_T.size();
    }

    void setMaxSize(const int maxSize)
    {
        QMutexLocker locker(&m_mutex);
        m_maxSize = maxSize;
    }

private:
    QQueue<T> m_T;
    mutable QMutex m_mutex;
    int m_maxSize;
};

}

#endif // TASKQUEUE_H
