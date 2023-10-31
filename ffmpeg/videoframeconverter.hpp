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
    explicit VideoFrameConverter(Frame *frame,
                                 const QSize &size = QSize(-1, -1),
                                 AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA,
                                 QObject *parent = nullptr);
    ~VideoFrameConverter() override;

    void flush(Frame *frame,
               const QSize &dstSize = QSize(-1, -1),
               AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA);

    auto scale(Frame *in, Frame *out) -> int;

    static auto isSupportedInput_pix_fmt(AVPixelFormat pix_fmt) -> bool;
    static auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool;

private:
    class VideoFrameConverterPrivate;
    QScopedPointer<VideoFrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg
