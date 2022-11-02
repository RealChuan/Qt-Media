#ifndef FRAMECONVERTER_HPP
#define FRAMECONVERTER_HPP

#include <QObject>
#include <QSize>

namespace Ffmpeg {

class CodecContext;
class PlayFrame;

// AV_PIX_FMT_RGB32
class FrameConverter : public QObject
{
public:
    explicit FrameConverter(CodecContext *codecCtx,
                            const QSize &size = QSize(-1, -1),
                            QObject *parent = nullptr);
    ~FrameConverter();

    void flush(PlayFrame *frame, const QSize &dstSize = QSize(-1, -1));

    int scale(PlayFrame *in, PlayFrame *out, int height);

    QImage scaleToImageRgb32(PlayFrame *in,
                             PlayFrame *out,
                             CodecContext *codecCtx,
                             const QSize &dstSize = QSize(-1, -1));

private:
    struct FrameConverterPrivate;
    QScopedPointer<FrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FRAMECONVERTER_HPP
