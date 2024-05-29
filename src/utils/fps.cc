#include "fps.hpp"

#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QQueue>

namespace Utils {

class Fps::FpsPrivate
{
public:
    explicit FpsPrivate(Fps *q)
        : q_ptr(q)
    {
        reset();
    }

    void update()
    {
        QMutexLocker locker(&mutex);
        auto now = QDateTime::currentMSecsSinceEpoch();
        timePoints.enqueue(now);
        if (timePoints.size() > maxQueueSize) {
            timePoints.dequeue();
        }
    }

    auto getFps() -> float
    {
        QMutexLocker locker(&mutex);
        double fps = 0;
        if (timePoints.size() < 2) {
            return fps;
        }
        auto diff = timePoints.last() - timePoints.first();
        if (diff <= 0) {
            return fps;
        }
        fps = (timePoints.size() - 1) * 1000.0 / diff;
        return fps;
    }

    void reset()
    {
        QMutexLocker locker(&mutex);
        timePoints.clear();
    }

    Fps *q_ptr;

    int maxQueueSize = 100;
    QQueue<qint64> timePoints;
    QMutex mutex;
};

Fps::Fps(int maxQueueSize, QObject *parent)
    : QObject{parent}
    , d_ptr(new FpsPrivate(this))
{
    setMaxQueueSize(maxQueueSize);
}

Fps::~Fps() = default;

void Fps::setMaxQueueSize(int maxQueueSize)
{
    Q_ASSERT(maxQueueSize > 0);
    d_ptr->maxQueueSize = maxQueueSize;
}

void Fps::update()
{
    d_ptr->update();
}

auto Fps::getFps() -> float
{
    return d_ptr->getFps();
}

void Fps::reset()
{
    d_ptr->reset();
}

} // namespace Utils
