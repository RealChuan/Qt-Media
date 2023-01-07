#pragma once

#include <QObject>
#include <QSize>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Frame;
class CodecContext;

class VideoFrameConverter : public QObject
{
public:
    explicit VideoFrameConverter(CodecContext *codecCtx,
                                 const QSize &size = QSize(-1, -1),
                                 AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA,
                                 QObject *parent = nullptr);
    VideoFrameConverter(Frame *frame,
                        const QSize &size = QSize(-1, -1),
                        AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA,
                        QObject *parent = nullptr);
    ~VideoFrameConverter();

    void flush(Frame *frame,
               const QSize &dstSize = QSize(-1, -1),
               AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA);

    int scale(Frame *in, Frame *out);

    static bool isSupportedInput_pix_fmt(AVPixelFormat pix_fmt);
    static bool isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt);

private:
    void setError(int errorCode);
    inline void debugMessage();

    struct VideoFrameConverterPrivate;
    QScopedPointer<VideoFrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg
