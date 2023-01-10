#include "fps.hpp"

#include <QDebug>
#include <QElapsedTimer>

namespace Utils {

class Fps::FpsPrivate
{
public:
    FpsPrivate(QObject *parent)
        : owner(parent)
    {}

    float fps(int deltaTime) // ms
    {
        ++frameCount;
        if (1 == frameCount) {
            avgDuration = static_cast<float>(deltaTime);
        } else {
            avgDuration = avgDuration * (1 - alpha) + deltaTime * alpha;
        }
        return (1.f / avgDuration * 1000);
    }

    float calculate()
    {
        float fps_ = 0;
        if (timer.isValid()) {
            fps_ = fps(timer.elapsed());
        }
        timer.restart();
        return fps_;
    }

    void reset()
    {
        avgDuration = 0.f;
        frameCount = 0;
        timer.invalidate();
    }

    QObject *owner;

    float avgDuration = 0.f;
    float alpha = 1.f / 100.f; // 采样数设置为100
    int frameCount = 0;
    QElapsedTimer timer;
};

Fps::Fps(QObject *parent)
    : QObject{parent}
    , d_ptr(new FpsPrivate(this))
{}

Fps::~Fps() {}

float Fps::calculate()
{
    return d_ptr->calculate();
}

void Fps::reset()
{
    d_ptr->reset();
}

} // namespace Utils
