#ifndef SPEED_HPP
#define SPEED_HPP

#include "utils_global.h"

#include <QObject>

namespace Utils {

class UTILS_EXPORT Speed : public QObject
{
public:
    explicit Speed(int interval = 5, QObject *parent = nullptr);
    ~Speed() override;

    void setInterval(int interval); // second

    void addSize(qint64 size);

    auto getSpeed() -> qint64; // per second

    void reset();

private:
    class SpeedPrivate;
    QScopedPointer<SpeedPrivate> d_ptr;
};

} // namespace Utils

#endif // SPEED_HPP
