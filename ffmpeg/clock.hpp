#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <QObject>

namespace Ffmpeg {

class Clock : public QObject
{
public:
    explicit Clock(QObject *parent = nullptr);
    ~Clock() override;

    void reset();

    auto pts() -> qint64;
    auto ptsDrift() -> qint64;
    auto lastUpdated() -> qint64;

    void resetSerial();
    auto serial() -> qint64;

    void setPaused(bool value);
    auto paused() -> bool;

    void update(qint64 pts, qint64 time);

    // return true if delay is valid
    auto getDelayWithMaster(qint64 &delay) -> bool;

    // return true if delay is valid
    static auto adjustDelay(qint64 &delay) -> bool;

    static void globalSerialRef();
    static void globalSerialReset();

    static auto speedRange() -> QPair<double, double>;
    static auto speedStep() -> double;
    static void setSpeed(double value);
    static auto speed() -> double;

    // not thread safe and not delete clock
    static void setMaster(Clock *clock);

private:
    class ClockPrivate;
    QScopedPointer<ClockPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CLOCK_HPP
