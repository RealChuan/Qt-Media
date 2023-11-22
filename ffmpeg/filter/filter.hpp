#ifndef FILTER_HPP
#define FILTER_HPP

#include "frame.hpp"
#include <QObject>

namespace Ffmpeg {

class AVContextInfo;
class Frame;
class Filter : public QObject
{
    Q_OBJECT
public:
    explicit Filter(AVContextInfo *decContextInfo, QObject *parent = nullptr);
    ~Filter() override;

    auto init(Frame *frame) -> bool;

    auto filterFrame(Frame *frame) -> QVector<FramePtr>;

private:
    class FilterPrivate;
    QScopedPointer<FilterPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTER_HPP
