#ifndef FILTER_HPP
#define FILTER_HPP

#include <ffmpeg/colorutils.hpp>
#include <mediaconfig/equalizer.hpp>
#include <videorender/tonemap.hpp>

#include <QObject>

extern "C" {
#include <libavutil/avutil.h>
}

namespace Ffmpeg {

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

    auto filterFrame(Frame *frame) -> QVector<QSharedPointer<Frame>>;

    auto buffersinkCtx() -> FilterContext *;

    static auto scale(const QSize &size) -> QString;
    static auto ep(const MediaConfig::Equalizer &equalizer) -> QString;
    static auto zscale(ColorUtils::Primaries::Type destPrimaries, Tonemap::Type type) -> QString;

private:
    class FilterPrivate;
    QScopedPointer<FilterPrivate> d_ptr;
};

using FilterPtr = QSharedPointer<Filter>;

} // namespace Ffmpeg

#endif // FILTER_HPP
