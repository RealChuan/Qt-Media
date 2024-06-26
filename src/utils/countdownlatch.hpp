#ifndef COUNTDOWNLATCH_HPP
#define COUNTDOWNLATCH_HPP

#include "utils_global.h"

#include <QScopedPointer>

namespace Utils {

class UTILS_EXPORT CountDownLatch
{
    Q_DISABLE_COPY_MOVE(CountDownLatch)
public:
    explicit CountDownLatch(int count);
    ~CountDownLatch();

    void wait();
    void countDown();

    void setCount(int count);
    [[nodiscard]] auto getCount() const -> int;

private:
    struct CountDownLatchPrivate;
    QScopedPointer<CountDownLatchPrivate> d_ptr;
};

} // namespace Utils

#endif // COUNTDOWNLATCH_HPP
