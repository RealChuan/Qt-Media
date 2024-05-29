#ifndef FILTERINOUT_HPP
#define FILTERINOUT_HPP

#include <QObject>

struct AVFilterInOut;

namespace Ffmpeg {

class FilterInOut : public QObject
{
    Q_OBJECT
public:
    explicit FilterInOut(QObject *parent = nullptr);
    ~FilterInOut() override;

    auto avFilterInOut() -> AVFilterInOut *;
    void setAVFilterInOut(AVFilterInOut *avFilterInOut);

private:
    class FilterInOutPrivate;
    QScopedPointer<FilterInOutPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTERINOUT_HPP
