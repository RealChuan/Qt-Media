#ifndef FILTER_HPP
#define FILTER_HPP

#include "frame.hpp"
#include <QObject>

namespace Ffmpeg {

class AVContextInfo;
class Frame;
class FilterContext;
class Filter : public QObject
{
    Q_OBJECT
public:
    explicit Filter(QObject *parent = nullptr);
    ~Filter() override;

    [[nodiscard]] auto isInitialized() const -> bool;

    auto init(AVMediaType type, Frame *frame) -> bool;
    // default args:
    // Video is "null"
    // Audio is "anull"
    void config(const QString &filterSpec);

    auto filterFrame(Frame *frame) -> QVector<FramePtr>;

    auto buffersinkCtx() -> FilterContext *;

private:
    class FilterPrivate;
    QScopedPointer<FilterPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FILTER_HPP
