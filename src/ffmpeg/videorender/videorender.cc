#include "videorender.hpp"

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

    ~VideoRenderPrivate() = default;

    void flushFPS() const { fpsPtr->update(); }

    void resetFps() const { fpsPtr->reset(); }

    QScopedPointer<Utils::Fps> fpsPtr;
};

VideoRender::VideoRender()
    : d_ptr(new VideoRenderPrivate)
{}

VideoRender::~VideoRender() = default;

void VideoRender::setFrame(FramePtr framePtr)
{
    auto *avFrame = framePtr->avFrame();
    if (avFrame->width <= 0 || avFrame->height <= 0) {
        return;
    }
    if (!isSupportedOutput_pix_fmt(static_cast<AVPixelFormat>(avFrame->format))) {
        framePtr = convertSupported_pix_fmt(framePtr);
    }
    if (nullptr == framePtr) {
        return;
    }
    updateFrame(framePtr);

    d_ptr->flushFPS();
}

void VideoRender::setImage(const QImage &image)
{
    if (image.isNull()) {
        return;
    }
    FramePtr framePtr(Frame::fromQImage(image));
    setFrame(framePtr);
}

void VideoRender::setSubTitleFrame(const QSharedPointer<Subtitle> &framePtr)
{
    if (framePtr->image().isNull()) {
        return;
    }
    updateSubTitleFrame(framePtr);
}

auto VideoRender::fps() -> float
{
    return d_ptr->fpsPtr->getFps();
}

void VideoRender::resetFps()
{
    d_ptr->resetFps();
}

} // namespace Ffmpeg
