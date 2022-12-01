#ifndef FRAMECONVERTER_HPP
#define FRAMECONVERTER_HPP

#include <QImage>
#include <QObject>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Frame;
class CodecContext;

class FrameConverter : public QObject
{
public:
    explicit FrameConverter(CodecContext *codecCtx,
                            const QSize &size = QSize(-1, -1),
                            AVPixelFormat pix_fmt = AV_PIX_FMT_RGB32,
                            QObject *parent = nullptr);
    FrameConverter(Frame *frame,
                   const QSize &size = QSize(-1, -1),
                   AVPixelFormat pix_fmt = AV_PIX_FMT_RGB32,
                   QObject *parent = nullptr);
    ~FrameConverter();

    void flush(Frame *frame,
               const QSize &dstSize = QSize(-1, -1),
               AVPixelFormat pix_fmt = AV_PIX_FMT_RGB32);

    int scale(Frame *in, Frame *out, int height);

    QImage scaleToQImage(Frame *in,
                         Frame *out,
                         const QSize &dstSize = QSize(-1, -1),
                         QImage::Format format = QImage::Format_RGB32);

    static bool isSupportedInput_pix_fmt(AVPixelFormat pix_fmt);
    static bool isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt);

private:
    struct FrameConverterPrivate;
    QScopedPointer<FrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // FRAMECONVERTER_HPP
