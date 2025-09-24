#pragma once

#include "frame.hpp"

#include <QObject>
#include <QSize>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class CodecContext;

class VideoFrameConverter : public QObject
{
public:
    explicit VideoFrameConverter(CodecContext *codecCtx,
                                 const QSize &size = QSize(-1, -1),
                                 AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA,
                                 QObject *parent = nullptr);
    explicit VideoFrameConverter(const FramePtr &framePtr,
                                 const QSize &size = QSize(-1, -1),
                                 AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA,
                                 QObject *parent = nullptr);
    ~VideoFrameConverter() override;

    void setColorspaceDetails(const FramePtr &framePtr,
                              float brightness,
                              float contrast,
                              float saturation);
    void flush(const FramePtr &framePtr,
               const QSize &dstSize = QSize(-1, -1),
               AVPixelFormat pix_fmt = AV_PIX_FMT_RGBA);

    auto scale(const FramePtr &inPtr, const FramePtr &outPtr) -> int;

    static auto isSupportedInput_pix_fmt(AVPixelFormat pix_fmt) -> bool;
    static auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool;

private:
    class VideoFrameConverterPrivate;
    QScopedPointer<VideoFrameConverterPrivate> d_ptr;
};

} // namespace Ffmpeg
