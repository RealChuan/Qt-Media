#ifndef FPS_HPP
#define FPS_HPP

#include "utils_global.h"

#include <QObject>

namespace Utils {

class UTILS_EXPORT Fps : public QObject
{
public:
    explicit Fps(int maxQueueSize = 100, QObject *parent = nullptr);
    ~Fps() override;

    void setMaxQueueSize(int maxQueueSize);

    void update();

    auto getFps() -> float; // ms

    void reset();

private:
    class FpsPrivate;
    QScopedPointer<FpsPrivate> d_ptr;
};

} // namespace Utils

#endif // FPS_HPP
