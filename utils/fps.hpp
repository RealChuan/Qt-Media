#ifndef FPS_HPP
#define FPS_HPP

#include "utils_global.h"

#include <QObject>

namespace Utils {

class UTILS_EXPORT Fps : public QObject
{
public:
    explicit Fps(QObject *parent = nullptr);
    ~Fps();

    float calculate(); // ms

    void reset();

private:
    class FpsPrivate;
    QScopedPointer<FpsPrivate> d_ptr;
};

} // namespace Utils

#endif // FPS_HPP
