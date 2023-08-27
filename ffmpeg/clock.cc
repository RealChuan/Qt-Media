#include "clock.hpp"

#include <QDebug>
#include <QMutex>

extern "C" {
#include <libavutil/time.h>
}

namespace Ffmpeg {

class Clock::ClockPrivate
{
public:
    explicit ClockPrivate(Clock *q)
        : q_ptr(q)
    {}

    Clock *q_ptr;

    mutable QMutex mutex;
    qint64 pts = 0;       // 当前 AVFrame 的时间戳 microseconds
    qint64 pts_drift = 0; // 时钟漂移量，用于计算当前时钟的状态 microseconds
    qint64 last_updated = av_gettime_relative(); // 上一次更新时钟状态的时间 microseconds
    qint64 serial = s_serial.load();             // 时钟序列号 for seek
    bool paused = false;                         // 是否暂停播放

    static std::atomic<qint64> s_serial;
    static constexpr QPair<double, double> s_speedRange = {0.5, 3.0};
    static constexpr auto s_speedStep = 0.5;
    static std::atomic<double> s_speed;
    static constexpr auto s_diffThreshold = 50 * 1000; // 50 milliseconds
    static Clock *s_clock;
};

std::atomic<qint64> Clock::ClockPrivate::s_serial = 0;
std::atomic<double> Clock::ClockPrivate::s_speed = 1.0;
Clock *Clock::ClockPrivate::s_clock = nullptr;

Clock::Clock(QObject *parent)
    : QObject{parent}
    , d_ptr(new ClockPrivate(this))
{}

Clock::~Clock() = default;

void Clock::reset()
{
    QMutexLocker locker(&d_ptr->mutex);
    d_ptr->pts = 0;
    d_ptr->pts_drift = 0;
    d_ptr->last_updated = av_gettime_relative();
    d_ptr->serial = Clock::ClockPrivate::s_serial.load();
    d_ptr->paused = false;
}

auto Clock::pts() -> qint64
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->pts;
}

auto Clock::ptsDrift() -> qint64
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->pts_drift;
}

auto Clock::lastUpdated() -> qint64
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->last_updated;
}

void Clock::resetSerial()
{
    QMutexLocker locker(&d_ptr->mutex);
    d_ptr->serial = Clock::ClockPrivate::s_serial.load();
}

auto Clock::serial() -> qint64
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->serial;
}

auto Clock::paused() -> bool
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->paused;
}

void Clock::setPaused(bool value)
{
    QMutexLocker locker(&d_ptr->mutex);
    d_ptr->paused = value;
    if (!d_ptr->paused) {
        d_ptr->last_updated = av_gettime_relative();
    }
}

void Clock::update(qint64 pts, qint64 time)
{
    Q_ASSERT(Clock::ClockPrivate::s_clock);

    QMutexLocker locker(&d_ptr->mutex);
    if (d_ptr->last_updated && !d_ptr->paused) {
        if (this == Clock::ClockPrivate::s_clock) {
            qint64 timediff = (time - d_ptr->last_updated) * speed();
            d_ptr->pts_drift += pts - d_ptr->pts - timediff;
        } else {
            auto *masterClock = Clock::ClockPrivate::s_clock;
            auto masterClockPts = masterClock->d_ptr->pts - masterClock->d_ptr->pts_drift;
            qint64 timediff = (time - masterClock->d_ptr->last_updated) * speed();
            d_ptr->pts_drift = pts - masterClockPts - timediff;
        }
    }
    d_ptr->pts = pts;
    d_ptr->last_updated = time;
}

auto Clock::getDelayWithMaster(qint64 &delay) -> bool
{
    if (serial() != Clock::ClockPrivate::s_serial.load()) {
        return false;
    }
    delay = ptsDrift();
    return true;
}

auto Clock::adjustDelay(qint64 &delay) -> bool
{
    if (speed() > 1.0 && delay < 0) {
        return false;
    } else if (delay < -Clock::ClockPrivate::s_diffThreshold) {
        return false;
    } else if (qAbs(delay) <= Clock::ClockPrivate::s_diffThreshold) {
        delay = 0;
        return true;
    }
    delay -= Clock::ClockPrivate::s_diffThreshold;
    return true;
}

void Clock::globalSerialRef()
{
    Clock::ClockPrivate::s_serial.fetch_add(1);
}

void Clock::globalSerialReset()
{
    Clock::ClockPrivate::s_serial.store(0);
}

auto Clock::speedRange() -> QPair<double, double>
{
    return Clock::ClockPrivate::s_speedRange;
}

auto Clock::speedStep() -> double
{
    return Clock::ClockPrivate::s_speedStep;
}

void Clock::setSpeed(double value)
{
    Clock::ClockPrivate::s_speed.store(value);
}

auto Clock::speed() -> double
{
    return Clock::ClockPrivate::s_speed.load();
}

void Clock::setMaster(Clock *clock)
{
    Clock::ClockPrivate::s_clock = clock;
}

} // namespace Ffmpeg
