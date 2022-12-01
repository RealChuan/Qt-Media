#ifndef VIDEORENDER_HPP
#define VIDEORENDER_HPP

#include <ffmpeg/ffmepg_global.h>

#include <QSharedPointer>

extern "C" {
#include <libavutil/pixfmt.h>
}

namespace Ffmpeg {

class Frame;

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual bool isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) = 0;
    virtual QVector<AVPixelFormat> supportedOutput_pix_fmt() = 0;
    virtual void convertSupported_pix_fmt(QSharedPointer<Frame> frame) = 0;
    void setFrame(QSharedPointer<Frame> frame);

protected:
    // may use in anthoer thread, suggest use QMetaObject::invokeMethod(Qt::QueuedConnection)
    virtual void updateFrame(QSharedPointer<Frame> frame) = 0;
};

} // namespace Ffmpeg

#endif // VIDEORENDER_HPP
