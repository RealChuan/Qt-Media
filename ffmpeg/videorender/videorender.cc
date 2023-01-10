#include "videorender.hpp"

#include <ffmpeg/frame.hpp>
#include <ffmpeg/subtitle.h>
#include <utils/fps.hpp>

#include <QDebug>
#include <QElapsedTimer>

extern "C" {
#include <libavformat/avformat.h>
}

namespace Ffmpeg {

class VideoRender::VideoRenderPrivate
{
public:
    VideoRenderPrivate()
        : fpsPtr(new Utils::Fps)
    {}

    ~VideoRenderPrivate() {}

    void flushFPS() { fps = fpsPtr->calculate(); }

    void resetFps()
    {
        fps = 0;
        fpsPtr->reset();
    }

    QScopedPointer<Utils::Fps> fpsPtr;
    float fps = 0;
};

VideoRender::VideoRender()
    : d_ptr(new VideoRenderPrivate)
{}

VideoRender::~VideoRender() {}

void VideoRender::setFrame(QSharedPointer<Frame> frame)
{
    auto avFrame = frame->avFrame();
    if (avFrame->width <= 0 || avFrame->height <= 0) {
        return;
    }

    if (!isSupportedOutput_pix_fmt(AVPixelFormat(frame->avFrame()->format))) {
        frame = convertSupported_pix_fmt(frame);
    }
    updateFrame(frame);
    //qDebug() << frame->avFrame()->format;

    d_ptr->flushFPS();
}

void VideoRender::setImage(const QImage &image)
{
    if (image.isNull()) {
        return;
    }
    QSharedPointer<Frame> frame(new Frame(image));
    setFrame(frame);
}

void VideoRender::setSubTitleFrame(QSharedPointer<Subtitle> frame)
{
    if (frame->image().isNull()) {
        return;
    }
    updateSubTitleFrame(frame);
}

float VideoRender::fps()
{
    return d_ptr->fps;
}

void VideoRender::resetFps()
{
    d_ptr->resetFps();
}

} // namespace Ffmpeg
