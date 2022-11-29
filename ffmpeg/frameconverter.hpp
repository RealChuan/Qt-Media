#ifndef FRAMECONVERTER_HPP
#define FRAMECONVERTER_HPP

#include <QObject>
#include <QSize>

namespace Ffmpeg {

class CodecContext;
class Frame;

// AV_PIX_FMT_RGB32
class FrameConverter : public QObject
{
public:
    explicit FrameConverter(CodecContext *codecCtx,
                            const QSize &size = QSize(-1, -1),
                            QObject *parent = nullptr);
    ~FrameConverter();

    void flush(Frame *frame, const QSize &dstSize = QSize(-1, -1));

    int scale(Frame *in, Frame *out, int height);

    QImage scaleToImageRgb32(Frame *in,
                             Frame *out,
                             CodecContext *codecCtx,
                             const QSize &dstSize = QSize(-1, -1));

private:
    struct FrameConverterPrivate;
    QScopedPointer<FrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FRAMECONVERTER_HPP
