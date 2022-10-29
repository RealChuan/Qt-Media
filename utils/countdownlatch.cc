#include "countdownlatch.hpp"

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>

namespace Utils {

struct CountDownLatch::CountDownLatchPrivate
{
    mutable QMutex mutex;
    QWaitCondition waintCondition;
    int count;
};

CountDownLatch::CountDownLatch(int count)
    : d_ptr(new CountDownLatchPrivate)
{
    d_ptr->count = count;
}

CountDownLatch::~CountDownLatch() {}

void CountDownLatch::wait()
{
    QMutexLocker locker(&d_ptr->mutex);
    while (d_ptr->count > 0) {
        d_ptr->waintCondition.wait(&d_ptr->mutex);
    }
}

void CountDownLatch::countDown()
{
    QMutexLocker locker(&d_ptr->mutex);
    d_ptr->count--;
    if (d_ptr->count == 0) {
        d_ptr->waintCondition.wakeAll();
    }
}

int CountDownLatch::getCount() const
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->count;
}

} // namespace Utils
