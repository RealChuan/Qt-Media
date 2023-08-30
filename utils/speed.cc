#include "speed.hpp"

#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QQueue>

namespace Utils {

class Speed::SpeedPrivate
{
public:
    explicit SpeedPrivate(Speed *q)
        : q_ptr(q)
    {
        reset();
    }

    void addSize(qint64 size)
    {
        QMutexLocker locker(&mutex);
        auto now = QDateTime::currentMSecsSinceEpoch();
        sizePoints.enqueue({size, now});
        auto threshold = (interval + 1) * 1000;
        auto diff = sizePoints.last().time - sizePoints.first().time;
        while (diff >= threshold) {
            sizePoints.dequeue();
            if (sizePoints.isEmpty()) {
                return;
            }
        }
        diff = sizePoints.last().time - sizePoints.first().time;
    }

    auto getSpeed() -> qint64
    {
        QMutexLocker locker(&mutex);
        qint64 speed = 0;
        if (sizePoints.size() < 2) {
            return speed;
        }
        auto diff = sizePoints.last().time - sizePoints.first().time;
        if (diff <= 0) {
            return speed;
        }
        qint64 sum = 0;
        for (int i = 1; i < sizePoints.size(); ++i) {
            sum += sizePoints[i].size;
        }
        speed = sum * 1000 / diff;
        return speed;
    }

    void reset()
    {
        QMutexLocker locker(&mutex);
        sizePoints.clear();
    }

    struct SizePoint
    {
        qint64 size = 0;
        qint64 time = 0;
    };

    Speed *q_ptr;

    int interval = 5;
    QQueue<SizePoint> sizePoints;
    QMutex mutex;
};

Speed::Speed(int interval, QObject *parent)
    : QObject{parent}
    , d_ptr(new SpeedPrivate(this))
{
    setInterval(interval);
}

Speed::~Speed() = default;

void Speed::setInterval(int interval)
{
    Q_ASSERT(interval > 0);
    d_ptr->interval = interval;
}

void Speed::addSize(qint64 size)
{
    Q_ASSERT(size > 0);
    d_ptr->addSize(size);
}

auto Speed::getSpeed() -> qint64
{
    return d_ptr->getSpeed();
}

void Speed::reset()
{
    d_ptr->reset();
}

} // namespace Utils
