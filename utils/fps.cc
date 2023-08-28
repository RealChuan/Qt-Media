#include "fps.hpp"

#include <QDateTime>
#include <QDebug>
#include <QQueue>

namespace Utils {

class Fps::FpsPrivate
{
public:
    FpsPrivate(Fps *q)
        : q_ptr(q)
    {
        reset();
    }

    auto update() -> float
    {
        auto now = QDateTime::currentMSecsSinceEpoch();
        timePoints.enqueue(now);
        if (timePoints.size() > maxQueueSize) {
            timePoints.dequeue();
        }
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

    void reset() { timePoints.clear(); }

    Fps *q_ptr;

    int maxQueueSize = 100;
    QQueue<qint64> timePoints;
};

Fps::Fps(QObject *parent)
    : QObject{parent}
    , d_ptr(new FpsPrivate(this))
{}

Fps::~Fps() {}

float Fps::calculate()
{
    return d_ptr->update();
}

void Fps::reset()
{
    d_ptr->reset();
}

} // namespace Utils
