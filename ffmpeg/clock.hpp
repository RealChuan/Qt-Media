#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <QObject>

namespace Ffmpeg {

class Clock : public QObject
{
public:
    explicit Clock(QObject *parent = nullptr);
    ~Clock() override;

    void reset(qint64 pts);

    void invalidate();
    [[nodiscard]] auto isVaild() const -> bool;

    [[nodiscard]] auto pts() const -> qint64;
    [[nodiscard]] auto ptsDrift() const -> qint64;
    [[nodiscard]] auto lastUpdated() const -> qint64;

    void resetSerial();
    [[nodiscard]] auto serial() const -> qint64;

    void setPaused(bool value);
    [[nodiscard]] auto paused() const -> bool;

    void update(qint64 pts, qint64 time);

    // return true if delay is valid
    auto getDelayWithMaster(qint64 &delay) const -> bool;

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
    static auto master() -> Clock *;

private:
    class ClockPrivate;
    QScopedPointer<ClockPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // CLOCK_HPP
