#ifndef VIDEORENDER_HPP
#define VIDEORENDER_HPP

#include <ffmpeg/ffmepg_global.h>

#include <QSharedPointer>

extern "C" {
#include <libavutil/pixfmt.h>
}

class QWidget;

namespace Ffmpeg {

class Frame;
class Subtitle;

class FFMPEG_EXPORT VideoRender
{
    Q_DISABLE_COPY_MOVE(VideoRender)
public:
    VideoRender();
    virtual ~VideoRender();

    virtual auto isSupportedOutput_pix_fmt(AVPixelFormat pix_fmt) -> bool = 0;
    virtual QVector<AVPixelFormat> supportedOutput_pix_fmt() = 0;
    virtual auto convertSupported_pix_fmt(QSharedPointer<Frame> framePtr) -> QSharedPointer<Frame>
        = 0;
    void setFrame(QSharedPointer<Frame> framePtr);
    void setImage(const QImage &image);
    void setSubTitleFrame(QSharedPointer<Subtitle> framePtr);
    virtual void resetAllFrame() = 0;

    virtual auto widget() -> QWidget * = 0;

    auto fps() -> float;
    void resetFps();

protected:
    // may use in anthoer thread, suggest use QMetaObject::invokeMethod(Qt::QueuedConnection)
    virtual void updateFrame(QSharedPointer<Frame> frame) = 0;
    virtual void updateSubTitleFrame(QSharedPointer<Subtitle> frame) = 0;

private:
    class VideoRenderPrivate;
    QScopedPointer<VideoRenderPrivate> d_ptr;
};

} // namespace Ffmpeg

#endif // VIDEORENDER_HPP
